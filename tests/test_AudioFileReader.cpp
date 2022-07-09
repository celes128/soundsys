#include "pch.h"
#include "../soundsys/AudioFileReader.h"
#include <chrono>
#include <array>

TEST(AudioFileReader, OneRead)
{
	std::vector<byte> data(512, (byte)0xAB);

	std::ofstream	ofs("temp.bin", std::ios::binary);
	ofs.write((const char*)data.data(), data.size());
	ofs.close();

	std::ifstream	file("temp.bin", std::ios::binary);

	sound::AudioFileReader	r(512, &file);
	auto err = r.Read();
	EXPECT_FALSE(err);

	// Copy the data in a vector.
	std::vector<byte> got(512, 0);
	std::memcpy(got.data(), r.Data().ptr, r.Data().size);

	EXPECT_EQ(got, data);

	file.close();
}

const size_t CHUNKSIZE = 512;
using Chunk = std::array<byte, CHUNKSIZE>;
using ChunkList = std::vector<Chunk>;

static void write_to_file(const ChunkList &chunks, const std::string filepath)
{
	std::ofstream	ofs("temp.bin", std::ios::binary);

	for (const auto ch : chunks) {
		ofs.write((const char*)ch.data(), ch.size());
	}

	ofs.close();
}

static ChunkList read_file(sound::AudioFileReader &reader)
{
	ChunkList chunks;

	while (true) {
		auto err = reader.Read();
		if (err) {
			break;
		}

		Chunk ch;
		std::memcpy((void*)ch.data(), reader.Data().ptr, reader.Data().size);

		chunks.push_back(ch);
	}

	return chunks;
}

TEST(AudioFileReader, Padding)
{
	// Generate some data.
	std::vector<byte> expected(256, 0xFF);

	// Write it to a file.
	const auto filepath = std::string("temp.bin");
	std::ofstream	ofs(filepath, std::ios::binary);
	ofs.write((const char*)expected.data(), expected.size());
	ofs.close();

	// Read the entire file into a list of chunks.
	std::ifstream	file(filepath, std::ios::binary);
	sound::AudioFileReader	reader(512, &file);

	reader.Read();
	auto data = reader.Data();
	EXPECT_EQ(data.size, 512);

	// The first 256 bytes match the original data.
	std::vector<byte> got(256);
	std::memcpy(got.data(), reader.Data().ptr, 256);
	EXPECT_EQ(got, expected);

	// The last 256 bytes are zeros.
	std::memcpy(got.data(), reader.Data().ptr + 256, 256);
	std::vector<byte> zeros(256, 0);
	EXPECT_EQ(got, zeros);

	file.close();
}

TEST(AudioFileReader, ReadEntireFile)
{
	// Generate some data chunks.
	ChunkList	expected;
	for (int i = 0; i < 2; i++) {
		Chunk ch;
		std::fill(ch.begin(), ch.end(), (byte)i);

		expected.push_back(ch);
	}

	// Write the data to a file.
	const auto filepath = std::string("temp.bin");
	write_to_file(expected, filepath);
	
	// Read the entire file into a list of chunks.
	std::ifstream	file(filepath, std::ios::binary);
	sound::AudioFileReader	reader(CHUNKSIZE, &file);
	auto got = read_file(reader);
	file.close();

	EXPECT_EQ(got, expected);
}