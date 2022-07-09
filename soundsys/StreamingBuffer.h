#pragma once

#include "framework.h"
#include <array>

#include "Range.h"

// TOOD: replace code that uses Result with Error.
using Result = int;

const Result RESULT_OK = 1;
const Result RESULT_FAILURE = -1;

namespace sound {

	class StreamingBuffer {
	public:
		DISALLOW_COPY_AND_ASSIGN(StreamingBuffer);

		StreamingBuffer(LPDIRECTSOUND8 directSound, int numSeconds, std::array<int, 2> notifyPositions);

		~StreamingBuffer();


		//					ACCESSORS
		//

		// Capacity returns the maximum number of bytes that can be stored in the buffer.
		DWORD Capacity() const { return m_desc.dwBufferBytes; }

		// RegionSize returns the number of bytes occupied by a region.
		//
		// PRECONDTIONS
		//	region == 0 or 1
		//
		DWORD RegionSize(int region);

		// RegionStart returns the byte offset of the region from the start of the buffer.
		//
		// PRECONDTIONS
		//	region == 0 or 1
		//
		DWORD RegionStart(int region);

		// 
		bool APositionWasSignaled(int *pos);


		//					MANIPULATORS
		//

		// Starts to generate sound from the audio data.
		void Play();

		// Stops to generate sound from the audio data.
		void Stop();

		// WriteToRegion fills either region 0 or 1 with bytes.
		//
		// PRECONDITIONS
		//	region == 0 || region == 1
		//
		// INPUT
		//	void *src:
		//		Address of the first byte to copy.
		//		The amount of bytes copied depends on the region number.
		Result WriteToRegion(int region, const byte *src);

		// Write fills a span of bytes in the buffer.
		Result Write(DWORD dest, const byte *src, DWORD size);

	private:
		//					CONSTRUCTION
		//

		// These functions are only called by the constructor.
		// They all throw if an error occured.
		void CreateWAVFormat();
		void CreateDesc();
		void CreateRegions();
		void CreateUnderlyingDirectSoundBuffer(LPDIRECTSOUND8 directSound);
		void CreateEvents();
		void SendNotificationPositions();
		

		//					REGIONS
		//

		//
		struct MemorySpan {
			LPVOID	begin{ nullptr };
			DWORD	length{ 0 };
		};

		struct DSBufferMemoryRegion {
			std::array<MemorySpan, 2>	span;
		};

		//// LockRegion locks a region of memory in the DirectSound buffer
		//// in order to write to it.
		//Result LockRegion(IN int region, OUT DSBufferMemoryRegion *memory);

		// LockMemory locks a span of memory in the buffer.
		Result LockMemory(IN DWORD offset, IN DWORD size, DSBufferMemoryRegion *memory);

		Result UnlockMemory(const DSBufferMemoryRegion &memory);

	private:
		// The underlying DirectSound buffer.
		LPDIRECTSOUNDBUFFER		m_DSBuffer{ nullptr };

		// Number of seconds that corresponds to the buffer capacity.
		int				m_numSeconds;
		
		WAVEFORMATEX	m_wavFormat;
		DSBUFFERDESC	m_desc;

		std::array<HANDLE, 2>	m_events{ 0, 0 };
		
		// Notification positions.
		// Expressed in percentage of the "buffer duration".
		// Valid range: from 0 to 100.
		// EXAMPLE
		//	25 means 25%
		std::array<int, 2>	m_notifPos;

		// The two regions defined the two notification positions.
		//
		// 0%             25%                              75%              100%
		// |---------------|--------------------------------|----------------|
		// |               |                                |                |
		// |    Region 1   |            Region 0            |    Region 1    |
		// |               |                                |                |
		//  ---------------|--------------------------------|----------------
		//               pos 0                            pos 1
		//
		using Region = Range<DWORD>;
		std::array<Region, 2>		m_regions;
	};
}