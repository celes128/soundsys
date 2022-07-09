#pragma once

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"dsound.lib")
#pragma comment(lib,"dxguid.lib")

#include "framework.h"

#include "StreamingBuffer.h"

// Music Request Queue
#include <ppl.h>
#include <concurrent_queue.h>
#include "MusicRequest.h"

#include "AudioFileReader.h"

namespace sound {

	class SoundSystem;

	Error	CreateSoundSystem(IN HWND window, OUT SoundSystem **system);
	void	DestroySoundSystem(IN OUT SoundSystem **system);

	class SoundSystem {
	public:
		DISALLOW_COPY_AND_ASSIGN(SoundSystem);

		//			MANIPULATORS
		//

		// Play tries to opens a music file and play its content.
		Error Play(const char *filename);


	private:
		// Creation and destruction is managed by the CreateSoundSystem and DestroySoundSystem functions.
		friend Error	CreateSoundSystem(IN HWND window, OUT SoundSystem **system);
		friend void		DestroySoundSystem(IN OUT SoundSystem **system);
		SoundSystem(HWND window);
		~SoundSystem();

		// CheckMusicRequest looks if the music request queue is not empty and
		// handles one request.
		//
		// RETURN VALUE
		//	Returns true iff a request was handled. If that is the case then there is
		// no need to check for an update in the sound buffer.
		//
		bool CheckMusicRequest();

		// CheckSoundBufferUpdate looks if a notification position in the streaming buffer
		// was signaled. If so, the function transfers audio data from the file to the buffer.
		// The function does nothing if there is no music playing.
		void CheckSoundBufferUpdate();

		// StopPlaying stops the current music being played by the sound buffer and
		// closes the associated audio file.
		void StopPlaying();

		void StartPlaying();

		// TransferOneDataChuck process a next audio data chunk that will be written
		// in a buffer region.
		// The data is first read from the file, then some fading effect can be applied
		// if fading is enabled and, finally, the data is copied into a buffer's region.
		// The region depends on the signaled position parameter.
		void TransferOneDataChuck(int sigPos);

		// RegionToUpdate returns the region that can safely receives a data chunk
		// based on which notification position was signaled.
		int RegionToUpdate(int sigPos);

		// OnReadError handles an read error from the audio file reader.
		// EOF is considered as a read error.
		// If the error is not EOF, this function will stop the music.
		void OnReadError(Error err, int sigPos);

		void OnEOF(int sigPos);

		// HandlePlayRequest handles a MusicRequest of type PLAY.
		Error HandlePlayRequest(const char *filename);

	private:
		//					STREAMING PROCEDURE
		//

		// A Windows timer to constantly call the streaming procedure.
		static const UINT_PTR	kTimerId = 1;
		static void CALLBACK	StreamingProcedure(HWND unnamed1, UINT unnamed2, UINT_PTR unnamed3, DWORD unnamed4);


		HWND					m_window{ nullptr };
		LPDIRECTSOUND8			m_directSound{ nullptr };
		LPDIRECTSOUNDBUFFER		m_primaryBuffer{ nullptr };

		//		Streaming
		//
		
		StreamingBuffer		*m_streamingBuffer{ nullptr };
		bool				m_playing{ false };
		int					m_sigPosAtEOF{ 0 };
		bool				m_atEOF{ false };

		using MusicRequestQueue = concurrency::concurrent_queue<MusicRequest>;
		MusicRequestQueue	m_requests;

		std::string				m_filename;
		std::ifstream			m_audioFile;
		sound::AudioFileReader	m_fileReader;
	};
}