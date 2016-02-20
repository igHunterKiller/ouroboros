// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/algo.h>
#include <oCore/color.h>
#include <oMath/hlslx.h>
#include <oMath/quantize.h>
#include <atomic>

using namespace std;
using namespace std::placeholders;

namespace ouro { namespace surface {

void put(const subresource_info_t& subresource_info, const mapped_subresource& dst, const uint2& coord, uint32_t argb)
{
	const int elSize = element_size(format(subresource_info.format));
	uint8_t* p = (uint8_t*)dst.data + (coord.y * dst.row_pitch) + (coord.x * elSize);

	argb_channels ch;
	ch.argb = argb;
	switch (subresource_info.format)
	{
		case format::r8g8b8a8_unorm: *p++ = ch.r; *p++ = ch.g; *p++ = ch.b; *p++ = ch.a; break;
		case format::r8g8b8_unorm:   *p++ = ch.r; *p++ = ch.g; *p++ = ch.b;              break;
		case format::b8g8r8a8_unorm: *p++ = ch.b; *p++ = ch.g; *p++ = ch.r; *p++ = ch.a; break;
		case format::b8g8r8_unorm:   *p++ = ch.b; *p++ = ch.g; *p++ = ch.r;              break;
		case format::r8_unorm:       *p++ = ch.r;                                        break;
		default: oThrow(std::errc::invalid_argument, "unsupported format");
	}
}

uint32_t get(const subresource_info_t& subresource_info, const const_mapped_subresource& src, const uint2& coord)
{
	const int elSize = element_size(subresource_info.format);
	const uint8_t* p = (const uint8_t*)src.data + (coord.y * src.row_pitch) + (coord.x * elSize);
	
	argb_channels ch;
	ch.r = 0;
	ch.g = 0;
	ch.b = 0;
	ch.a = 255;

	switch (subresource_info.format)
	{
		case format::r8g8b8a8_unorm: ch.r = *p++; ch.g = *p++; ch.b = *p++; ch.a = *p++; break;
		case format::r8g8b8_unorm:   ch.r = *p++; ch.g = *p++; ch.b = *p++;              break;
		case format::b8g8r8a8_unorm: ch.b = *p++; ch.g = *p++; ch.r = *p++; ch.a = *p++; break;
		case format::b8g8r8_unorm:   ch.b = *p++; ch.g = *p++; ch.r = *p++;              break;
		case format::r8_unorm:       ch.r = *p++; ch.g = ch.r; ch.b = ch.r;              break;
		default: break;
	}
	
	return ch.argb;
}

bool use_large_pages(const info_t& info, const uint2& tile_dimensions, uint32_t small_page_size_bytes, uint32_t large_page_size_bytes)
{
	oCheck(!is_planar(info.format), std::errc::not_supported, "planar formats not supported");

	auto surface_size = mip_size(info.format, info.dimensions);
	if (surface_size < (large_page_size_bytes / 4))
		return false;

	auto pitch = row_pitch(info);

	// number of rows before we get a page miss
	float num_page_rows = small_page_size_bytes / static_cast<float>(pitch);

	auto tile_byte_width = row_size(info.format, tile_dimensions);
	
	// estimate how many bytes we would work on in a tile before encountering a 
	// tlb cache miss... not precise, but should be close enough for our purpose 
	// here. 
	float bytes_per_tlb_miss = tile_byte_width * num_page_rows - FLT_EPSILON;
	
	// If we are not going to get at least half a small page size of work done per tlb miss, better to use large pages instead.
	return bytes_per_tlb_miss <= (small_page_size_bytes / 2);
}

void enumerate_pixels(const uint2& dimensions, const format& format
	, const const_mapped_subresource& mapped
	, const function<void(const void* pixel)>& enumerator)
{
	auto row = (const uint8_t*)mapped.data;
	auto end = dimensions.y * mapped.row_pitch + row;
	const uint32_t format_size = element_size(format);
	for (; row < end; row += mapped.row_pitch)
	{
		auto pixel = row;
		auto row_end = dimensions.x * format_size + pixel;
		for (; pixel < row_end; pixel += format_size)
			enumerator(pixel);
	}
}

void enumerate_pixels(const uint2& dimensions, const format& format
	, const mapped_subresource& mapped
	, const function<void(void* pixel)>& enumerator)
{
	auto row = (uint8_t*)mapped.data;
	auto end = dimensions.y * mapped.row_pitch + row;
	const uint32_t format_size = element_size(format);
	for (; row < end; row += mapped.row_pitch)
	{
		auto pixel = row;
		auto row_end = dimensions.x * format_size + pixel;
		for (; pixel < row_end; pixel += format_size)
			enumerator(pixel);
	}
}

// Calls the specified function on each pixel of two same-formatted images.
void enumerate_pixels(const uint2& dimensions, const format& format
	, const const_mapped_subresource& mapped1
	, const const_mapped_subresource& mapped2
	, const function<void(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2)>& enumerator)
{
	auto row1 = (const uint8_t*)mapped1.data;
	auto row2 = (const uint8_t*)mapped2.data;
	auto end1 = dimensions.y * mapped1.row_pitch + row1;
	const uint32_t format_size = element_size(format);
	while (row1 < end1)
	{
		auto pixel1 = row1;
		auto pixel2 = row2;
		auto row_end1 = dimensions.x * format_size + pixel1;
		while (pixel1 < row_end1)
		{
			enumerator(pixel1, pixel2);
			pixel1 += format_size;
			pixel2 += format_size;
		}

		row1 += mapped1.row_pitch;
		row2 += mapped2.row_pitch;
	}
}

void enumerate_pixels(const info_t& input_info
	, const const_mapped_subresource& mappedInput1
	, const const_mapped_subresource& mappedInput2
	, const info_t& output_info
	, mapped_subresource& mapped_output
	, const function<void(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2, void* oRESTRICT out_pixel)>& enumerator)
{
	if (any(input_info.dimensions != output_info.dimensions))
		oThrow(std::errc::invalid_argument, "Dimensions mismatch In(%dx%d) != Out(%dx%d)"
			, input_info.dimensions.x
			, input_info.dimensions.y
			, output_info.dimensions.x
			, output_info.dimensions.y);
	
	const void* pRow1 = mappedInput1.data;
	const void* pRow2 = mappedInput2.data;
	const void* pEnd1 = byte_add(pRow1, input_info.dimensions.y * mappedInput1.row_pitch);
	void* pRowOut = mapped_output.data;
	const int InputFormatSize = element_size(input_info.format);
	const int OutputFormatSize = element_size(output_info.format);
	while (pRow1 < pEnd1)
	{
		const void* pPixel1 = pRow1;
		const void* pPixel2 = pRow2;
		const void* pRowEnd1 = byte_add(pPixel1, input_info.dimensions.x * InputFormatSize);
		void* pOutPixel = pRowOut;
		while (pPixel1 < pRowEnd1)
		{
			enumerator(pPixel1, pPixel2, pOutPixel);
			pPixel1 = byte_add(pPixel1, InputFormatSize);
			pPixel2 = byte_add(pPixel2, InputFormatSize);
			pOutPixel = byte_add(pOutPixel, OutputFormatSize);
		}

		pRow1 = byte_add(pRow1, mappedInput1.row_pitch);
		pRow2 = byte_add(pRow2, mappedInput2.row_pitch);
		pRowOut = byte_add(pRowOut, mapped_output.row_pitch);
	}
}

typedef void (*rms_enumerator)(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2, void* oRESTRICT out_pixel, atomic<uint32_t>* inout_accum);

static void sum_squared_diff_r8_to_r8(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2, void* oRESTRICT out_pixel, atomic<uint32_t>* inout_accum)
{
	const uint8_t* p1 = (const uint8_t*)pixel1;
	const uint8_t* p2 = (const uint8_t*)pixel2;
	uint8_t absDiff = uint8_t (abs(*p1 - *p2));
	*(uint8_t*)out_pixel = absDiff;
	inout_accum->fetch_add(uint32_t(absDiff * absDiff));
}

static void sum_squared_diff_b8g8r8_to_r8(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2, void* oRESTRICT out_pixel, atomic<uint32_t>* inout_accum)
{
	const uint8_t* p = (const uint8_t*)pixel1;
	argb_channels ch;
	ch.b = *p++;
	ch.g = *p++;
	ch.r = *p++;
	ch.a = 0xff;

	float4 srgb = truetofloat4(ch.argb);
	float L1 = srgbtolum(srgb.xyz());
	
	p = (const uint8_t*)pixel2;
	ch.b = *p++;
	ch.g = *p++;
	ch.r = *p++;

	srgb = truetofloat4(ch.argb);
	float L2 = srgbtolum(srgb.xyz());
	uint8_t absDiff = (uint8_t)f32ton8(abs(L1 - L2));
	*(uint8_t*)out_pixel = absDiff;
	inout_accum->fetch_add(uint32_t(absDiff * absDiff));
}

static void sum_squared_diff_b8g8r8a8_to_r8(const void* oRESTRICT pixel1, const void* oRESTRICT pixel2, void* oRESTRICT out_pixel, atomic<uint32_t>* inout_accum)
{
	const uint8_t* p = (const uint8_t*)pixel1;
	argb_channels ch;
	ch.b = *p++;
	ch.g = *p++;
	ch.r = *p++;
	ch.a = *p++;

	float4 srgb = truetofloat4(ch.argb);
	float L1 = srgbtolum(srgb.xyz());

	p = (const uint8_t*)pixel2;
	ch.b = *p++;
	ch.g = *p++;
	ch.r = *p++;
	ch.a = *p++;

	srgb = truetofloat4(ch.argb);
	float L2 = srgbtolum(srgb.xyz());
	uint8_t absDiff = (uint8_t)f32ton8(abs(L1 - L2));
	*(uint8_t*)out_pixel = absDiff;
	inout_accum->fetch_add(uint32_t(absDiff * absDiff));
}

static rms_enumerator get_rms_enumerator(format in_format, format out_format)
{
	#define CASE(i,o) ((uint32_t(i)<<16) | uint32_t(o))
	uint32_t req = CASE(in_format, out_format);

	switch (req)
	{
		case CASE(format::r8_unorm, format::r8_unorm): return sum_squared_diff_r8_to_r8;
		case CASE(format::b8g8r8_unorm, format::r8_unorm): return sum_squared_diff_b8g8r8_to_r8;
		case CASE(format::b8g8r8a8_unorm, format::r8_unorm): return sum_squared_diff_b8g8r8a8_to_r8;
		default: break;
	}

	oThrow(std::errc::not_supported, "%s -> %s not supported", as_string(in_format), as_string(out_format));
	#undef CASE
}

float calc_rms(const info_t& info
	, const const_mapped_subresource& mapped1
	, const const_mapped_subresource& mapped2)
{
	rms_enumerator en = get_rms_enumerator(info.format, format::r8_unorm);
	atomic<uint32_t> SumOfSquares(0);
	uint32_t DummyPixelOut[4]; // largest a pixel can ever be currently

	enumerate_pixels(info.dimensions.xy(), info.format
		, mapped1
		, mapped2
		, bind(en, _1, _2, &DummyPixelOut, &SumOfSquares));

	return sqrt(SumOfSquares / float(info.dimensions.x * info.dimensions.y));
}

float calc_rms(const info_t& input_info
	, const const_mapped_subresource& mappedInput1
	, const const_mapped_subresource& mappedInput2
	, const info_t& output_info
	, mapped_subresource& mapped_output)
{
	function<void(const void* pixel1, const void* pixel2, void* out_pixel)> Fn;

	rms_enumerator en = get_rms_enumerator(input_info.format, output_info.format);
	atomic<uint32_t> SumOfSquares(0);

	enumerate_pixels(input_info
		, mappedInput1
		, mappedInput2
		, output_info
		, mapped_output
		, bind(en, _1, _2, _3, &SumOfSquares));

	return sqrt(SumOfSquares / float(input_info.dimensions.x * input_info.dimensions.y));
}

typedef void (*histogramenumerator)(const void* pixel, atomic<uint32_t>* histogram);

static void histogram_r8_unorm_8bit(const void* pixel, atomic<uint32_t>* histogram)
{
	uint8_t c = *(const uint8_t*)pixel;
	histogram[c]++;
}

static void histogram_b8g8r8a8_unorm_8bit(const void* pixel, atomic<uint32_t>* histogram)
{
	const uint8_t* p = (const uint8_t*)pixel;
	argb_channels ch;
	ch.b = *p++;
	ch.g = *p++;
	ch.r = *p++;
	ch.a = 0xff;

	float4 srgb = truetofloat4(ch.argb);
	float lum = srgbtolum(srgb.xyz());
	uint8_t index = (uint8_t)f32ton8(lum);
	histogram[index]++;
}

static void histogram_r16_unorm_16bit(const void* pixel, atomic<uint32_t>* histogram)
{
	uint16_t c = *(const uint16_t*)pixel;
	histogram[c]++;
}

static void histogram_r16_float_16bit(const void* pixel, atomic<uint32_t>* histogram)
{
	float f = f16tof32(*(const uint16_t*)pixel);
	uint16_t c = static_cast<uint16_t>(round(65535.0f * f));
	histogram[c]++;
}

histogramenumerator get_histogramenumerator(const format& f, int bitdepth)
{
	#define IO(f,b) ((uint32_t(f) << 16) | uint32_t(b))
	uint32_t sel = IO(f, bitdepth);
	switch (sel)
	{
		case IO(format::r8_unorm, 8): return histogram_r8_unorm_8bit;
		case IO(format::b8g8r8a8_unorm, 8): return histogram_b8g8r8a8_unorm_8bit;
		case IO(format::r16_unorm, 16): return histogram_r16_unorm_16bit;
		case IO(format::r16_float, 16): return histogram_r16_float_16bit;
		default: break;
	}

	oThrow(std::errc::not_supported, "%dbit histogram on %s not supported", bitdepth, as_string(f));
	#undef IO
}

void histogram8(const subresource_info_t& subresource_info, const const_mapped_subresource& mapped, uint32_t histogram[256])
{
	atomic<uint32_t> H[256];
	memset(H, 0, sizeof(uint32_t) * 256);
	histogramenumerator en = get_histogramenumerator(subresource_info.format, 8);
	enumerate_pixels(subresource_info.dimensions.xy(), subresource_info.format, mapped, bind(en, _1, H));
	memcpy(histogram, H, sizeof(uint32_t) * 256);
}

void histogram16(const subresource_info_t& subresource_info, const const_mapped_subresource& mapped, uint32_t histogram[65536])
{
	atomic<uint32_t> H[65536];
	memset(H, 0, sizeof(uint32_t) * 65536);
	histogramenumerator en = get_histogramenumerator(subresource_info.format, 16);
	enumerate_pixels(subresource_info.dimensions.xy(), subresource_info.format, mapped, bind(en, _1, H));
	memcpy(histogram, H, sizeof(uint32_t) * 65536);
}

}}
