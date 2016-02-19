// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oBase/compression.h>
#include <zlib/zlib.h>

namespace ouro {

// GZip format
// 1 byte ID1 - 0x1f
// 1 byte ID2 - 0x8b
// 1 byte CM - 0x08  // means use deflate for compression which is the only allowed "option"
// 1 byte FLG - 0x00 // series of 8 flags, all should be 0 for our case. most are outdated, or not useful, or reserved
// 4 bytes MTIME - 0x00 // Modification time. last time file was modified, use 0 for http since its not really relevant in that case. even in regular file case 0 is fine
// 1 byte XFL - 0x02 // means use max compression. 0x04 is the only other option which means use fast compression. no direct translation to deflate values, just informational, not used for anything
// 1 byte OS - 0xff // there is a list of os's that you can specify. list created at time GZip was created. so not really relevant today. could use 11 for NTFS I suppose. 0xff means unknown
// many bytes - then place the deflate data
// 4 bytes CRC32 generate crc32 and place here
// 4 bytes ISIZE size of the data uncompressed

//Note that GZip is little endian

static const int GZipFooterSize = 8;
static const uint8_t GZipID1 = 0x1f;
static const uint8_t GZipID2 = 0x8b;
static const uint8_t GZipCM = 0x08;

static const uint8_t GZipFlgFExtra = 0x04;
static const uint8_t GZipFlgFName = 0x08;
static const uint8_t GZipFlgFComment = 0x10;
static const uint8_t GZipFlgFHCrc = 0x02;

#pragma pack(1)
struct GZIP_HDR
{
	uint8_t ID1;
	uint8_t ID2;
	uint8_t CM;
	uint8_t FLG;
	uint32_t MTIME;
	uint8_t XFL;
	uint8_t OS;
};

size_t compress_gzip(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT src, size_t src_size)
{
	size_t CompressedSize = 0;

	if (dst)
	{
		const size_t EstSize = compress_gzip(nullptr, 0, nullptr, src_size);
		oCheck(!dst || dst_size >= EstSize, std::errc::no_buffer_space, "");

		GZIP_HDR h;
		h.ID1 = GZipID1;
		h.ID2 = GZipID2;
		h.CM = GZipCM;
		h.FLG = 0x00;
		h.MTIME = 0;
		h.XFL = 0x02;
		h.OS = 0xff;
		uint32_t ISIZE = (uint32_t)src_size;

		memcpy(dst, &h, sizeof(h));
		dst_size -= sizeof(h);
		dst = (uint8_t*)dst + sizeof(h);
	
		uLongf bytesWritten = (uLongf)dst_size;
		int result = compress2(static_cast<Bytef*>(dst), &bytesWritten, static_cast<const Bytef*>(src), static_cast<uint32_t>(src_size), 9);
		oCheck(result == Z_OK, std::errc::protocol_error, "compression failed");

		dst_size -= bytesWritten;
		dst = (uint8_t*)dst + bytesWritten;
		oCheck(dst_size >= GZipFooterSize, std::errc::no_buffer_space, "");

		uint32_t& CRC32 = *(uint32_t*)dst;
		CRC32 = crc32(0, Z_NULL, 0);
		CRC32 = crc32(CRC32, static_cast<const Bytef*>(src), static_cast<uint32_t>(src_size));

		dst_size -= sizeof(CRC32);
		dst = (uint8_t*)dst + sizeof(CRC32);
		*(uint32_t*)dst = static_cast<uint32_t>(src_size);

		CompressedSize = sizeof(h) + bytesWritten + GZipFooterSize;
	}

	else
		CompressedSize = compressBound(static_cast<uint32_t>(src_size)) + sizeof(GZIP_HDR) + GZipFooterSize;
	
	return CompressedSize;
}

size_t decompress_gzip(void* oRESTRICT dst, size_t dst_size, const void* oRESTRICT in_src, size_t src_size)
{
	const void* src = in_src;
	const GZIP_HDR& h = *(const GZIP_HDR*)src;
	oCheck(h.ID1 == GZipID1 && h.ID2 == GZipID2 && h.CM == GZipCM, std::errc::protocol_error, "Not a valid GZip stream");
	src = (const uint8_t*)src + sizeof(GZIP_HDR);

	// we don't generate optional fields but we need to skip over them if someone 
	// else did
	if (h.FLG && GZipFlgFExtra)
	{
		uint16_t extraSize = *(uint16_t*)src;
		src = (const uint8_t*)src + sizeof(uint16_t) + extraSize;
	}

	if (h.FLG && GZipFlgFName)
	{
		while (*static_cast<const char*>(src))
			src = (const uint8_t*)src + 1;
	}

	if (h.FLG && GZipFlgFComment)
	{
		while (*static_cast<const char*>(src))
			src = (const uint8_t*)src + 1;
	}

	if (h.FLG && GZipFlgFHCrc)
		src = (const uint8_t*)src + 2;

	size_t sz = src_size - size_t((const uint8_t*)src - (const uint8_t*)in_src) - GZipFooterSize;
	uint32_t compressedSize = (uint32_t)sz;
	uint32_t UncompressedSize = *(uint32_t*)((const uint8_t*)in_src + sizeof(h) + compressedSize + sizeof(uint32_t));

	if (dst)
	{
		oCheck(!dst || dst_size >= UncompressedSize, std::errc::no_buffer_space, "");
		uLongf bytesWritten = (uLongf)dst_size;
		oCheck(Z_OK == uncompress(static_cast<Bytef*>(dst), &bytesWritten, static_cast<const Bytef*>(src), compressedSize), std::errc::protocol_error, "decompression error");
		uint32_t expectedCRC32 = crc32(0, Z_NULL, 0);
		expectedCRC32 = crc32(expectedCRC32, static_cast<const Bytef*>(dst), UncompressedSize);
		uint32_t CRC32 = *(uint32_t*)((const uint8_t*)in_src + sizeof(h) + compressedSize);
		oCheck(expectedCRC32 == CRC32, std::errc::protocol_error, "CRC mismatch in GZip stream");
	}

	return UncompressedSize;
}

}
