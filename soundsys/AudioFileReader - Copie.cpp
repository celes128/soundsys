#include "pch.h"
#include "AudioFileReader.h"

namespace sound {

	AudioFileReader::AudioFileReader(size_t bufCapacity, std::ifstream *file)
		: m_buf(bufCapacity, 0)
		, m_file(file)
	{
		assert(bufCapacity >= 1);
	}

	Error AudioFileReader::Read(size_t size)
	{
		assert(size <= BufferCapacity());

		m_dataSize = 0;

		if (size == 0) {
			size = BufferCapacity();
		}

		Error err;
		if (UnusualState(OUT &err)) {
			std::fill(m_buf.data(), m_buf.data() + size, (byte)0);// Fill with zeros
			m_dataSize = size;
			return err;
		}

		m_file->read((char *)m_buf.data(), size);
		auto numRead = m_file->gcount();

		if (0 < numRead && numRead < size) {
			std::fill(m_buf.data() + numRead, m_buf.data() + size, (byte)0);// Pad with zeros
			m_dataSize = size;
		}

		if (m_file->eof()) {
			return ERROR_EOF;
		}
		else if (m_file->bad() || m_file->fail()) {
			m_failure = true;
			return ERROR_READ;
		}
		else {
			m_dataSize = size;
		}
		
		return ERROR_NONE;
	}

	bool AudioFileReader::UnusualState(OUT Error *err)
	{
		assert(err != nullptr);

		if (!m_file) {
			*err = ERROR_FAILURE;
			return true;
		}

		if (Failure()) {
			*err = ERROR_READ;
			return true;
		}

		if (AtEOF()) {
			*err = ERROR_EOF;
			return true;
		}

		return false;
	}
}