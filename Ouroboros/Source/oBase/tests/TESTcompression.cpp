// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/finally.h>
#include <oBase/compression.h>
#include <oString/path.h>
#include <oCore/timer.h>

using namespace ouro;

static void TestCompress(const void* _pSourceBuffer, size_t _SizeofSourceBuffer, const compression& type, size_t* _pCompressedSize)
{
	size_t maxCompressedSize = compress(type, nullptr, 0, _pSourceBuffer, _SizeofSourceBuffer);
	void* compressed = new char[maxCompressedSize];
	oFinally { if (compressed) delete [] compressed; };

	*_pCompressedSize = compress(type, compressed, maxCompressedSize, _pSourceBuffer, _SizeofSourceBuffer);
	oCHECK(*_pCompressedSize != 0, "");

	size_t uncompressedSize = decompress(type, nullptr, 0, compressed, *_pCompressedSize);
	oCHECK(_SizeofSourceBuffer == uncompressedSize, "loaded and uncompressed sizes don't match");

	void* uncompressed = malloc(uncompressedSize);
	oFinally { if (uncompressed) free(uncompressed); };

	oCHECK(0 != decompress(type, uncompressed, uncompressedSize, compressed, *_pCompressedSize), "");
	oCHECK(!memcmp(_pSourceBuffer, uncompressed, uncompressedSize), "memcmp failed between uncompressed and loaded buffers");
}

oTEST(oBase_compression)
{
	static const char* TestPath = "Test/Geometry/buddha.obj";

	blob OBJBuffer = srv.load_buffer(TestPath);

	double timeSnappy, timeLZMA, timeGZip;
	size_t CompressedSize0, CompressedSize1, CompressedSize2;

	timer t;
	TestCompress(OBJBuffer, OBJBuffer.size(), compression::snappy, &CompressedSize0);
	timeSnappy = t.seconds();
	t.reset();
	TestCompress(OBJBuffer, OBJBuffer.size(), compression::lzma, &CompressedSize1);
	timeLZMA = t.seconds();
	t.reset();
	TestCompress(OBJBuffer, OBJBuffer.size(), compression::gzip, &CompressedSize2);
	timeGZip = t.seconds();
	t.reset();

	sstring strUncompressed, strSnappy, strLZMA, strGZip, strSnappyTime, strLZMATime, strGZipTime;
	format_bytes(strUncompressed, OBJBuffer.size(), 2);
	format_bytes(strSnappy, CompressedSize0, 2);
	format_bytes(strLZMA, CompressedSize1, 2);
	format_bytes(strGZip, CompressedSize2, 2);
	
	format_duration(strSnappyTime, timeSnappy, true);
	format_duration(strLZMATime, timeLZMA, true);
	format_duration(strGZipTime, timeGZip, true);

	srv.status("Compressed %s from %s to Snappy: %s in %s, LZMA: %s in %s, GZip: %s in %s"
		, TestPath
		, strUncompressed.c_str()
		, strSnappy.c_str()
		, strSnappyTime.c_str()
		, strLZMA.c_str()
		, strLZMATime.c_str()
		, strGZip.c_str()
		, strGZipTime.c_str());
}
