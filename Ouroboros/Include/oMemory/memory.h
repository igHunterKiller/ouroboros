// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// common advanced memory operations

#pragma once
#include <oArch/compiler.h>
#include <memory.h>
#include <cstdint>

namespace ouro {

// _____________________________________________________________________________
// memcmps

enum class utf_type
{
	unknown, 
	binary, 
	ascii, 
	utf8, 
	utf16be, 
	utf16le, 
	utf32be, 
	utf32le,
};

// Returns the UTF type of the specified buffer
utf_type utfcmp(const void* buf, size_t buf_size);

// Determination is similar to Perl's -T flag: if more than 10% of buf is either 
// 0 or non-ascii this returns false.
bool is_ascii(const void* buf, size_t buf_size);
inline bool is_binary(const void* buf, size_t buf_size) { return !is_ascii(buf, buf_size); }

// Compares a run of memory to a fixed value. Memory unaligned to the size of 
// value (sizeof(long)) will compare to the aligned byte portion of value. This
// means the memory is expected to have been set with memset4 which will always 
// have its 4-byte value start on 4-byte-aligned addresses. This function is 
// mostly useful when setting sentinel memory to a known shred value and then
// using this to test for the integrity of the sentinel. Returns true if memory
// is fully the same, or false if even one byte differs.
bool memcmp4(const void* mem, long value, size_t bytes);

// locate a binary substring, like Linux's memmem function.
void* memmem(void* buf, size_t buf_size, const void* find, size_t find_size);
inline const void* memmem(const void* buf, size_t buf_size, const void* find, size_t find_size) { return memmem(const_cast<void*>(buf), buf_size, find, find_size); }

// _____________________________________________________________________________
// memsets

// Sets an int at a time. This is probably slower than c's memset but the full 
// value is set rather than always a char value so that longer shred values may 
// be used such as 0xdeadbeef or 0xfeeefeee rather than memset's dede or fefe.
void memset2(void* dst, int16_t value, size_t bytes);
void memset4(void* dst, int32_t value, size_t bytes);
void memset8(void* dst, int64_t value, size_t bytes);

// Sets n bytes from source to destination for the specified copy size number 
// of bytes. This uses memset and above flavors if src_size is appropriate.
void memnset(void* oRESTRICT dst, const void* oRESTRICT src, size_t src_size, size_t copy_size);

// Versions for working with 2D memory where scanlines might not be contiguous.
void memset2d(void* dst, size_t row_pitch, int value, size_t set_pitch, size_t num_rows);
void memset2d2(void* dst, size_t row_pitch, int16_t value, size_t set_pitch, size_t num_rows);
void memset2d4(void* dst, size_t row_pitch, int32_t value, size_t set_pitch, size_t num_rows);
void memset2d8(void* dst, size_t row_pitch, int64_t value, size_t set_pitch, size_t num_rows);

inline void memset3d(void* dst, size_t row_pitch, size_t slice_pitch, int value, size_t set_pitch, size_t num_rows, size_t num_slices)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = slice_pitch * num_slices + d;
	for (; d < end; d += slice_pitch)
		memset2d(d, row_pitch, value, set_pitch, num_rows);
}

inline void memset3d2(void* dst, size_t row_pitch, size_t slice_pitch, int16_t value, size_t set_pitch, size_t num_rows, size_t num_slices)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = slice_pitch * num_slices + d;
	for (; d < end; d += slice_pitch)
		memset2d2(d, row_pitch, value, set_pitch, num_rows);
}

inline void memset3d4(void* dst, size_t row_pitch, size_t slice_pitch, int32_t value, size_t set_pitch, size_t num_rows, size_t num_slices)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = slice_pitch * num_slices + d;
	for (; d < end; d += slice_pitch)
		memset2d4(d, row_pitch, value, set_pitch, num_rows);
}

inline void memset3d8(void* dst, size_t row_pitch, size_t slice_pitch, int64_t value, size_t set_pitch, size_t num_rows, size_t num_slices)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = slice_pitch * num_slices + d;
	for (; d < end; d += slice_pitch)
		memset2d8(d, row_pitch, value, set_pitch, num_rows);
}

// Decodes PackBits-style run-length encoding (RLE) for an arbitrarily-sized
// repeat pattern as specified in size by element_size.
void rle_decode(void* oRESTRICT dst, size_t dst_size, size_t element_size, const void* oRESTRICT src);

// Similar to rle_decode. The header/count is still one byte, but the count 
// describes numbers of elements of rle_element_size (for things like multi-byte 
// pixel values). This returns where in src the decode left off.
const void* rle_decoden(void* oRESTRICT dst, size_t dst_size, 
	size_t element_stride, size_t rle_element_size, const void* oRESTRICT src);

// _____________________________________________________________________________
// 2D copies for copying image data, stuff that's easier to conceptualize as 2D 
// rather than 1D.

// dst: pointer to the start of the 2D box to write
// dst_pitch: # bytes to get to the point just below dst
// src: pointer to the start of the 2D box to read
// src_pitch: # bytes to get to the point just below src
// src_row_size: # bytes in a scanline to copy into dst
// num_rows: Number of scanline copies to perform.

// NOTE: src_row_size is different from src_pitch or dst_pitch 
// because this function can be used for copying a subregion out of the middle
// of a larger box. Thus the pitch to get to the next line might be 
// significantly larger than the valid data to copy. Likewise the destination
// could be in the middle of a larger container, so its pitch too might be 
// different than the data to be moved. In the case where an entire image or 
// surface is copied, src_pitch will likely be very similar or identical to
// src_row_size.
// Besides copying AOS to AOS you can also use this function to copy 
// asymmetrical. Set src_row_size to the same value as src_pitch (SOA to 
// AOS) or set src_row_size to the same value as dst_pitch (AOS to 
// SOA). If copying SOA-style data to AOS-style data, ensure dst is 
// pointing at the first field in the struct to write, not always the start of 
// the struct itself.
inline void memcpy2d(void* oRESTRICT dst, size_t dst_pitch, 
	const void* oRESTRICT src, size_t src_pitch, size_t src_row_size, size_t num_rows)
{
	if (dst_pitch == src_pitch && src_pitch == src_row_size)
		memcpy(dst, src, src_pitch * num_rows);
	else
	{
		uint8_t* oRESTRICT d = (uint8_t*)dst;
		const uint8_t* oRESTRICT s = (const uint8_t*)src;
		const uint8_t* oRESTRICT end = dst_pitch * num_rows + d;
		for (; d < end; d += dst_pitch, s += src_pitch)
			memcpy(d, s, src_row_size);
	}
}

// Just like memcpy2d, but copies from the last source scanline to the 
// first, thus flipping the image vertically for systems whose coordinate system 
// is flipped V from the destination coordinate system.
inline void memcpy2dvflip(void* oRESTRICT dst, size_t dst_pitch, 
	const void* oRESTRICT src, size_t src_pitch, size_t src_row_size, size_t num_rows)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = dst_pitch * num_rows + d;
	const uint8_t* oRESTRICT flipped_src = src_pitch * (num_rows-1) + (const uint8_t*)src;
	for (; d < end; d += dst_pitch, flipped_src -= src_pitch)
		memcpy(d, flipped_src, src_row_size);
}

// A version that optionally chooses between the two above functions
inline void memcpy2d(void* oRESTRICT dst, size_t dst_pitch, 
	const void* oRESTRICT src, size_t src_pitch, size_t src_row_size, 
	size_t num_rows, bool flip_vertically)
{
	if (flip_vertically) memcpy2dvflip(dst, dst_pitch, src, src_pitch, src_row_size, num_rows);
	else memcpy2d(dst, dst_pitch, src, src_pitch, src_row_size, num_rows);
}

// 3D memory is assumed to have the 2D slices adjacent in memory 
//
// |-A-|   A: row pitch
// +---+-  B: slice pitch
// | 0 |B
// +---+-  Just like there can be some bytes at the end of a row 
// | 1 |   that aren't part of the copy, there can be rows that 
// +---+   aren't part of a slice, so slice pitches can be 
// | 2 |   different like row pitches can be different. However
// +---+   the counts of the rows and slices must always match.
// |...|
// +---+
inline void memcpy3d(void* oRESTRICT dst, size_t dst_row_pitch, size_t dst_slice_pitch
	, const void* oRESTRICT src, size_t src_row_pitch, size_t src_slice_pitch
	, size_t src_row_size, size_t num_rows, size_t num_slices)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = dst_slice_pitch * num_slices + d;
	const uint8_t* oRESTRICT s = (const uint8_t*)src;
	for (; d < end; d += dst_slice_pitch, s += src_slice_pitch)
		memcpy2d(d, dst_row_pitch, s, src_row_pitch, src_row_size, num_rows);
}

inline void memcpy3dvflip(void* oRESTRICT dst, size_t dst_row_pitch, size_t dst_slice_pitch
	, const void* oRESTRICT src, size_t src_row_pitch, size_t src_slice_pitch
	, size_t src_row_size, size_t num_rows, size_t num_slices)
{
	uint8_t* oRESTRICT d = (uint8_t*)dst;
	const uint8_t* oRESTRICT end = dst_slice_pitch * num_slices + d;
	const uint8_t* oRESTRICT s = (const uint8_t*)src;
	for (; d < end; d += dst_slice_pitch, s += src_slice_pitch)
		memcpy2dvflip(d, dst_row_pitch, s, src_row_pitch, src_row_size, num_rows);
}

// A version that optionally chooses between the two above functions
inline void memcpy3dvflip(void* oRESTRICT dst, size_t dst_row_pitch, size_t dst_slice_pitch
	, const void* oRESTRICT src, size_t src_row_pitch, size_t src_slice_pitch
	, size_t src_row_size, size_t num_rows, size_t num_slices, bool flip_rows_vertically)
{
	if (flip_rows_vertically) memcpy3dvflip(dst, dst_row_pitch, dst_slice_pitch, src, src_row_pitch, src_slice_pitch, src_row_size, num_rows, num_slices);
	else memcpy3d(dst, dst_row_pitch, dst_slice_pitch, src, src_row_pitch, src_slice_pitch, src_row_size, num_rows, num_slices);
}

}
