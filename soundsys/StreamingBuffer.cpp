#include "pch.h"
#include "StreamingBuffer.h"
#include <cassert>

namespace sound {

	StreamingBuffer::StreamingBuffer(LPDIRECTSOUND8 directSound, int numSeconds,std::array<int, 2> notifyPositions)
		: m_numSeconds(numSeconds)
		, m_notifPos(notifyPositions)
	{
		assert(numSeconds >= 1);
		assert(notifyPositions[0] < notifyPositions[1]);
		assert(0 <= notifyPositions[0] && notifyPositions[0] <= 100);
		assert(0 <= notifyPositions[1] && notifyPositions[1] <= 100);

		// Each of these function will throw if an error occured.
		CreateWAVFormat();
		CreateDesc();
		CreateRegions();
		CreateUnderlyingDirectSoundBuffer(directSound);
		CreateEvents();
		SendNotificationPositions();
	}

	StreamingBuffer::~StreamingBuffer()
	{
		for (int i = 0; i < 2; ++i) {
			if (m_events[i]) {
				CloseHandle(m_events[i]);
				m_events[i] = nullptr;
			}
		}

		SafeRelease(&m_DSBuffer);
	}

	void StreamingBuffer::CreateWAVFormat()
	{
		// Format:	Mono - 44.1KHz - 16 bits per channel
		m_wavFormat = { 0 };
		m_wavFormat.wFormatTag = WAVE_FORMAT_PCM;
		m_wavFormat.nChannels = 1;
		m_wavFormat.nSamplesPerSec = 44100;
		m_wavFormat.wBitsPerSample = 16;
		m_wavFormat.nBlockAlign = (m_wavFormat.nChannels * m_wavFormat.wBitsPerSample) / 8;
		m_wavFormat.nAvgBytesPerSec = m_wavFormat.nSamplesPerSec * m_wavFormat.nBlockAlign;
		m_wavFormat.cbSize = 0;
	}

	void StreamingBuffer::CreateDesc()
	{
		assert(m_numSeconds >= 1);

		// Set the buffer description of the secondary sound buffer that the wave file will be loaded onto.
		m_desc = { 0 };
		m_desc.dwSize = sizeof(DSBUFFERDESC);
		m_desc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
		m_desc.lpwfxFormat = &m_wavFormat;

		// Buffer size
		m_desc.dwBufferBytes = m_numSeconds * m_wavFormat.nAvgBytesPerSec;
	}

	void StreamingBuffer::CreateRegions()
	{
		for (int i = 0; i < 2; i++) {
			auto percentage = m_notifPos[i] / 100.f;
			auto begin = static_cast<DWORD>(percentage * m_desc.dwBufferBytes);
			auto length = RegionSize(i);

			m_regions[i] = Region(begin, length);
		}
	}

	void StreamingBuffer::CreateUnderlyingDirectSoundBuffer(LPDIRECTSOUND8 directSound)
	{
		assert(directSound != nullptr);

		auto hr = directSound->CreateSoundBuffer(&m_desc, &m_DSBuffer, NULL);
		if (FAILED(hr)) {
			throw std::runtime_error("sound::StreamingBuffer - CreateSoundBuffer() failed.");
		}
	}

	void StreamingBuffer::CreateEvents()
	{
		// Create Windows events that will be signaled when the play cursor
		// crosses certain positions in the buffer.
		// We use two positions so we need two events, one for each position being crossed.
		for (int i = 0; i < 2; ++i) {
			m_events[i] = CreateEventA(NULL, FALSE, FALSE, NULL);
			if (!m_events[i]) {
				throw std::runtime_error("sound::StreamingBuffer - CreateEventA() failed.");
			}
		}
	}

	void StreamingBuffer::SendNotificationPositions()
	{
		const int N = 2;

		DSBPOSITIONNOTIFY pos[N];
		for (int i = 0; i < N; i++) {
			pos[i].dwOffset = m_regions[i].begin();
			pos[i].hEventNotify = m_events[i];
		}

		LPDIRECTSOUNDNOTIFY8 notify;
		if (FAILED(m_DSBuffer->QueryInterface(IID_IDirectSoundNotify, (LPVOID *)&notify))) {
			throw std::runtime_error("sound::StreamingBuffer - QueryInterface() failed.");
		}

		if (FAILED(notify->SetNotificationPositions(N, pos))) {
			throw std::runtime_error("sound::StreamingBuffer - SetNotificationPositions() failed.");
		}

		notify->Release();
	}



	bool StreamingBuffer::APositionWasSignaled(int *pos)
	{
		assert(pos != nullptr);

		auto hr = WaitForMultipleObjects(2, &m_events[0], FALSE, 0);
		if (WAIT_OBJECT_0 == hr) {
			*pos = 0;
			return true;
		}
		else if (WAIT_OBJECT_0 + 1 == hr) {
			*pos = 1;
			return true;
		}

		return false;
	}

	void StreamingBuffer::Play()
	{
		// Move the play cursor to the beginning of the buffer.
		m_DSBuffer->SetCurrentPosition(0);

		m_DSBuffer->Play(0, 0, DSBPLAY_LOOPING);
	}

	void StreamingBuffer::Stop()
	{
		m_DSBuffer->Stop();
	}

	Result StreamingBuffer::WriteToRegion(int region, const byte *src)
	{
		assert(region == 0 || region == 1);

		auto dest = RegionStart(region);
		auto size = RegionSize(region);
		auto res = Write(dest, src, size);

		assert(res == RESULT_OK);
		return res;
	}

	Result StreamingBuffer::Write(DWORD dest, const byte *src, DWORD size)
	{
		assert(0 <= dest && dest < Capacity());
		assert(0 <= size);

		DSBufferMemoryRegion	memory;
		auto res = LockMemory(IN dest, IN size, OUT &memory);
		if (res != RESULT_OK) {
			return RESULT_FAILURE;
		}

		// Write to the first part of the region.
		memcpy(
			memory.span[0].begin,				// destination
			src,								// source
			memory.span[0].length				// number of bytes to copy
		);

		// Optional write to the second part of the region.
		if (memory.span[1].begin != nullptr) {
			memcpy(
				memory.span[1].begin,			// destination
				src + memory.span[0].length,	// source
				memory.span[1].length			// number of bytes to copy
			);
		}

		UnlockMemory(memory);

		{
			// DEBUG
			static int counter = 0;
			counter = (counter + 1) % 2;

			if (counter) {
				DebugPrintfA("    ");
			}
			DebugPrintfA("(%d,%d); Write %d bytes at %d.\n", dest, size, memory.span[0].length, memory.span[0].begin);

			if (memory.span[1].begin) {
				// DEBUG
				if (counter) {
					DebugPrintfA("    ");
				}
				DebugPrintfA("(%d,%d); Write %d bytes at %d.\n", dest, size, memory.span[1].length, memory.span[1].begin);
			}
		}

		return RESULT_OK;
	}

	Result StreamingBuffer::UnlockMemory(const DSBufferMemoryRegion &memory)
	{
		auto hr = m_DSBuffer->Unlock(
			memory.span[0].begin, memory.span[0].length,	// first part
			memory.span[1].begin, memory.span[1].length		// second part
		);
		
		return FAILED(hr) ? RESULT_FAILURE : RESULT_OK;
	}

	Result StreamingBuffer::LockMemory(IN DWORD offset, IN DWORD size, DSBufferMemoryRegion *memory)
	{
		MemorySpan span0;
		MemorySpan span1;

		auto hr = m_DSBuffer->Lock(
			offset,
			size,
			&span0.begin, &span0.length,
			&span1.begin, &span1.length,
			0
		);

		memory->span[0] = span0;
		memory->span[1] = span1;

		return FAILED(hr) ? RESULT_FAILURE : RESULT_OK;
	}


	//					REGIONS
	//

	DWORD StreamingBuffer::RegionSize(int region)
	{
		assert(region == 0 || region == 1);

		float percentage = 0.f;

		switch (region) {
		case 0: {
			percentage = static_cast<float>((m_notifPos[1] - m_notifPos[0])) / 100.f;
		}break;

		case 1: {
			percentage = static_cast<float>(100 - (m_notifPos[1] - m_notifPos[0])) / 100.f;
		}break;
		}

		auto size = static_cast<DWORD>(
			(percentage * m_numSeconds) * m_wavFormat.nAvgBytesPerSec
			);
		return size;
	}

	DWORD StreamingBuffer::RegionStart(int region)
	{
		assert(region == 0 || region == 1);

		float percentage = 0.f;

		switch (region) {
		case 0: {
			percentage = m_notifPos[0] / 100.f;
		}break;

		case 1: {
			percentage = m_notifPos[1] / 100.f;
		}break;
		}

		auto offset = static_cast<DWORD>(percentage * Capacity());
		return offset;
	}
}