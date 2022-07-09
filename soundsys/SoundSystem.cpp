#include "pch.h"
#include "SoundSystem.h"
#include <stdexcept>

namespace sound {

	// File static pointer to the unique SoundSystem.
	// We need a global variable so that the streaming procedure can call functions of the class.
	static SoundSystem	*s_system;

	Error CreateSoundSystem(IN HWND window, OUT SoundSystem **system)
	{
		assert(system != nullptr);

		try {
			*system = new sound::SoundSystem(window);
		}
		catch (const std::exception &e) {
			return ERROR_FAILURE;
		}

		// Save the created instance in the file static variable.
		assert(s_system == nullptr);
		s_system = *system;

		return ERROR_NONE;
	}

	void DestroySoundSystem(IN OUT SoundSystem **system)
	{
		// I cannot use SafeDelete here because the SoundSystem destructor is private.
		if (*system) {
			delete *system;
			*system = nullptr;
		}

		// Clear the file static pointer to the system.
		s_system = nullptr;
	}

	SoundSystem::SoundSystem(HWND window)
		: m_window(window)
	{
		HRESULT hr = S_OK;

		hr = DirectSoundCreate8(NULL, &m_directSound, NULL);
		if (FAILED(hr)) {
			throw std::runtime_error("sound: DirectSoundCreate8() failed.");
		}

		hr = m_directSound->SetCooperativeLevel(m_window, DSSCL_PRIORITY);
		if (FAILED(hr)) {
			throw std::runtime_error("sound: SetCooperativeLevel() failed.");
		}

		// Create the primary buffer.
		DSBUFFERDESC desc{ 0 };
		ZeroMemory(&desc, sizeof(DSBUFFERDESC));
		desc.dwSize = sizeof(DSBUFFERDESC);
		desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
		desc.dwBufferBytes = 0;
		desc.lpwfxFormat = NULL;

		hr = m_directSound->CreateSoundBuffer(&desc, &m_primaryBuffer, NULL);
		if (FAILED(hr)) {
			throw std::runtime_error("sound: CreateSoundBuffer() failed.");
		}

		//m_primaryBuffer->Release();


		//					Streaming
		//

		// Create a ticking timer to call the streaming procedure every 300 milliseconds.
		if (0 == SetTimer(window, kTimerId, 300, &SoundSystem::StreamingProcedure)) {
			throw std::runtime_error("sound: (streaming) SetTimer() failed.");
		}

		// Create the streaming buffer:
		//	- big enough to hold 2 seconds of sound and,
		//	- two notification positions at 25% and 75%.
		m_streamingBuffer = new StreamingBuffer(m_directSound, 2, { 25, 75 });
	}

	SoundSystem::~SoundSystem()
	{
		SafeDelete(&m_streamingBuffer);

		KillTimer(m_window, kTimerId);

		m_primaryBuffer->Release();
		m_directSound->Release();
	}

	Error SoundSystem::Play(const char *filename)
	{
		m_requests.push(MakeMusicRequest_Play(filename));

		return ERROR_NONE;
	}

	bool SoundSystem::CheckMusicRequest()
	{
		// Pop one...
		MusicRequest req;
		auto gotOne = m_requests.try_pop(OUT req);
		if (!gotOne) {
			return false;
		}

		// ... and handle it.
		DebugPrintfA("\tHandling a request\n");
		switch (req.type) {
		case MUSIC_REQUEST_TYPE_PLAY: {
			HandlePlayRequest(req.filename);
		}break;

		case MUSIC_REQUEST_TYPE_STOP: {
			StopPlaying();
		}break;

		default: {
			assert(false && "Unknwon request or not yet implemented");
		}break;
		}

		return true;
	}

	void SoundSystem::CheckSoundBufferUpdate()
	{
		// There is nothing to do if is no song playing.
		if (!m_playing) {
			return;
		}

		int sigPos;
		auto signaled = m_streamingBuffer->APositionWasSignaled(&sigPos);
		if (!signaled) {
			return;
		}

		auto mustStopPlaying = m_atEOF && (sigPos == m_sigPosAtEOF);
		if (mustStopPlaying) {
			StopPlaying();
			return;
		}
		// NOTE
		// If we're at EOF and sigPos != sigPos,
		// the call below to transfer_one_data_chuck() will transfer a block of zeros to the sound buffer.

		TransferOneDataChuck(sigPos);
	}

	void SoundSystem::StopPlaying()
	{
		DebugPrintfA("Stop\n");
		m_streamingBuffer->Stop();

		m_audioFile.close();
		m_filename = "";

		m_playing = false;
	}

	void SoundSystem::StartPlaying()
	{
		DebugPrintfA("Play\n");
		m_streamingBuffer->Play();
		m_playing = true;
	}

	void SoundSystem::TransferOneDataChuck(int sigPos)
	{
		auto region = RegionToUpdate(sigPos);

		// Read a data chunk.
		auto size = m_streamingBuffer->RegionSize(region);
		auto err = m_fileReader.Read(size);
		if (err) {
			OnReadError(err, sigPos);
		}

		// TODO: Apply fading if enabled.

		// Write the data chunk.
		// We do not need to pass the data size; the streaming buffer will fill the entire region.
		m_streamingBuffer->WriteToRegion(region, m_fileReader.Data().ptr);
	}

	int SoundSystem::RegionToUpdate(int sigPos)
	{
		assert(sigPos == 0 || sigPos == 1);

		if (0 == sigPos) {
			return 1;
		}
		else {
			return 0;
		}
	}

	void SoundSystem::OnReadError(Error err, int sigPos)
	{
		assert(err != ERROR_NONE);

		if (err == ERROR_EOF) {
			if (!m_atEOF) {
				m_sigPosAtEOF = sigPos;
				m_atEOF = true;

				DebugPrintfA("EOF reached for the first time.\n");
			}
			return;
		}

		// Control flow reaching this part of the code means that
		// there was an unrecoverable read error.
		DebugPrintfA("ERROR: soundsys::read_next_audio_data() - Error while reading the audio file!\n");

		StopPlaying();
	}

	void SoundSystem::OnEOF(int sigPos)
	{
		m_sigPosAtEOF = sigPos;
		m_atEOF = true;
	}

	Error SoundSystem::HandlePlayRequest(const char *filename)
	{
		StopPlaying();

		// Open the audio file.
		m_audioFile = std::ifstream(filename, std::ios::binary);
		if (!m_audioFile) {
			m_audioFile.close();
			return ERROR_FAILURE;
		}
		m_filename = filename;

		// Transfer a data chunk big enough to fill the sound buffer up to
		// the start of region 1.
		// We also have to recreate the reader with the new audio file.
		auto size = static_cast<size_t>(m_streamingBuffer->RegionStart(1));
		m_fileReader = sound::AudioFileReader(size, &m_audioFile);
		
		auto err = m_fileReader.Read();
		if (err == ERROR_NONE) {
			m_atEOF = false;
		}
		else if (err == ERROR_EOF) {
			OnEOF(1);// position 1 is signaled
		}
		else {
			m_audioFile.close();
			return ERROR_FAILURE;
		}

		m_streamingBuffer->Write(0, m_fileReader.Data().ptr, m_fileReader.Data().size);

		StartPlaying();

		// TEST FADING OUT
		/*s_fadingEnabled = 1;
		sFadingEffect = sound::FadingEffect(sound::FADINGEFFECT_FADEOUT, 1000.0f);*/

		//DEBUG_fading_init();

		return ERROR_NONE;
	}


	//					STREAMING PROCEDURE
	//

	void CALLBACK SoundSystem::StreamingProcedure(HWND unnamed1, UINT unnamed2, UINT_PTR unnamed3, DWORD unnamed4)
	{
		assert(s_system != nullptr);

		OutputDebugStringA("Streaming procedure\n");

		auto handledOne = s_system->CheckMusicRequest();
		if (handledOne) {
			return;
		}

		s_system->CheckSoundBufferUpdate();
	}
}