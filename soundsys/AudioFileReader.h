#pragma once

#include "framework.h"
#include <fstream>
#include <vector>

namespace sound {

	// CLASS:		AudioFileReader 
	//
	// PURPOSE:		Reads chunks of data from an audio file into an internal buffer.
	//				When EOF is reached, the reader adds zero padding.
	//
	struct BufferData {
		const byte	*ptr;
		size_t		size;
	};

	class AudioFileReader {
	public:
		AudioFileReader(size_t bufCapacity = 64, std::ifstream *file = nullptr);

		//				ACCESSORS
		//

		// AtEOF returns true iff the reader reached EOF.
		bool AtEOF() const { return m_file->eof(); }

		// Failure returns true iff an error occured while reading the file.
		// Being at EOF is not a failure.
		auto Failure() const { return m_failure; }

		auto BufferCapacity() const { return m_buf.size(); }

		// Data returns a pointer to the data buffer and the number of bytes it contains.
		// The number of bytes includes the zero padding.
		const BufferData Data() const
		{
			return BufferData{ m_buf.data(), m_dataSize };
		}

		//				MANIPULATORS
		//

		// Read loads the buffer with the next bytes of audio.
		// It adds padding of zeros if EOF is reached.
		//
		// INPUT
		//	size_t size
		//		The number of bytes to read.
		//		If size == 0 then up to BufferCapacity() bytes are read.
		//
		// PRECONDITIONS
		//	size <= BufferCapacity()
		//
		Error Read(size_t size = 0);

	private:
		bool UnusualState(OUT Error *err);

		// ZeroData fills size bytes of the buffer with zeros and
		// sets the m_dataSize to size.
		void ZeroData(size_t size);

	private:
		std::vector<byte>	m_buf;
		size_t				m_dataSize{ 0 };

		std::ifstream		*m_file;

		bool				m_failure{ false };
	};
}