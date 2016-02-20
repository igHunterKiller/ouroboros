// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Utilities for specifying the basics of the ouro file format.

#pragma once
#include <cstdint>
#include <oBase/compression_type.h>
#include <oCore/fourcc.h>

namespace ouro {

// Ouro files are divided into chunks that map to specific memory allocations
struct file_chunk
{
  fourcc_t fourcc; // identifies chunk type
  uint32_t chunk_bytes; // size of chunk as it is in the file
  uint32_t uncompressed_bytes; // size of chunk if the buffer is uncompressed

	// returns a pointer to the next chunk after this one
	file_chunk* next() { return (file_chunk*)((uint8_t*)(this + 1) + chunk_bytes); }
	const file_chunk* next() const { return (const file_chunk*)((const uint8_t*)(this + 1) + chunk_bytes); }
	bool compressed() const { return chunk_bytes != uncompressed_bytes; }

	template<typename T> T* data() { return (T*)(this + 1); }
	template<typename T> const T* data() const { return (const T*)(this + 1); }

	bool in_range(const void* buffer, size_t size) const
	{
		auto ths = (const uint8_t*)this;
		auto buf = (const uint8_t*)buffer;
		return ths >= buffer && (ths + chunk_bytes) <= (buf + size);
	}
};
static_assert(sizeof(file_chunk) == 12, "size mismatch");

struct file_header
{
  fourcc_t fourcc; // identifies the file type
  uint8_t num_chunks; // counts how many file_chunks are in the file.
  compression_type compression; // each block that is compressed uses this compression algorithm
  uint16_t reserved; // 0
  uint64_t version_hash; // a hash ID that the runtime must match in order to process the file

	// returns a pointer to the first chunk in the file
	file_chunk* first_chunk() { return (file_chunk*)(this + 1); }
	const file_chunk* first_chunk() const { return (const file_chunk*)(this + 1); }

	const file_chunk* find_chunk(const fourcc_t& fcc) const
	{
		auto count = num_chunks;
		auto chk = first_chunk();
		while (count--)
		{
			if (chk->fourcc == fcc)
				return chk;
			chk = chk->next();
		}

		return nullptr;
	}

	file_chunk* find_chunk(const fourcc_t& fcc) { return const_cast<file_chunk*>(static_cast<const file_header*>(this)->find_chunk(fcc)); }

};
static_assert(sizeof(file_header) == 16, "size mismatch");

// Ouro files store strings (file paths or uris usually) as a uint16_t string length followed by
// the string and then a null terminator, so the byte size is 2 + strlen(str) + 1. The entire
// set of strings has one addition 0-length string to terminate the list.
struct file_pascal_string
{
	size_t length() const { return *(const uint16_t*)this; }
	const char* c_str() const { return (const char*)this + 2; }

	file_pascal_string* next() { return (file_pascal_string*)((uint8_t*)this + length() + 3); }
	const file_pascal_string* next() const { return (const file_pascal_string*)((const uint8_t*)this + length() + 3); }
};

inline void enumerate_pascal_strings(const file_chunk* string_chunk, void (*enumerator)(const char* string, void* user), void* user)
{
	const file_pascal_string* str = (const file_pascal_string*)(string_chunk + 1);
	while (str->length())
	{
		enumerator(str->c_str(), user);
		str = str->next();
	}
}

}
