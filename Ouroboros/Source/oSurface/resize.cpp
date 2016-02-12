// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/resize.h>
#include <oBase/fixed_vector.h>
#include <oMemory/memory.h>
#include <vector>
#include <float.h>

#define oSURF_CHECK(expr, format, ...) do { if (!(expr)) throw std::invalid_argument(format, ## __VA_ARGS__); } while(false)

namespace ouro {

const char* as_string(const surface::filter& f)
{
	switch (f)
	{
		case surface::filter::point: return "point";
		case surface::filter::box: return "box";
		case surface::filter::triangle: return "triangle";
		case surface::filter::lanczos2: return "lanczos2";
		case surface::filter::lanczos3: return "lanczos3";
		default: break;
	}
	return "?";
}

	namespace surface {

template<typename T> T sinc(T x)
{
	if (abs(x) > FLT_EPSILON)
	{
		x *= T(M_PI);
		return sin(x) / x;
	} 
	return T(1);
}

struct filter_point
{
	static const int kernel_width = 0;
protected:
	float value(float offset) const
	{
		return (abs(offset) < FLT_EPSILON) ? 1.0f : 0.0f;
	}
};

struct filter_box
{
	static const int kernel_width = 1;
protected:
	float value(float offset) const
	{
		return (abs(offset) <= 1.0f) ? 1.0f : 0.0f;
	}
};

struct filter_triangle
{
	static const int kernel_width = 1;
protected:
	float value(float offset) const
	{
		float abs_offset = abs(offset);
		if (abs_offset <= 1.0f)
			return 1.0f - abs_offset;
		else
			return 0.0f;
	}
};

struct filter_lanczos2
{
	static const int kernel_width = 2;
protected:
	float value(float offset) const
	{
		const float inv_width = 1.0f / kernel_width;
		float abs_offset = abs(offset);
		if (abs_offset <= kernel_width)
			return sinc(offset) * sinc(offset * inv_width);
		else
			return 0.0f;
	}
};

struct filter_lanczos3
{
	static const int kernel_width = 3;
protected:
	float value(float offset) const
	{
		const float inv_width = 1.0f/kernel_width;
		float abs_offset = abs(offset);
		if (abs_offset <= kernel_width)
			return sinc(offset) * sinc(offset*inv_width);
		else
			return 0.0f;
	}
};

template<typename T>
struct filter_t : public T
{
	static const int support = 2 * kernel_width + 1;
	struct entry_t
	{
		fixed_vector<float, support> cache;
		int left, right;
	};
	std::vector<entry_t> FilterCache;

	void initialize_filter(int srcDim, int dstDim)
	{
		FilterCache.resize(dstDim);

		//this is a hack for magnification. need to widen the filter since our source is actually discrete, but the math is mostly continuous, otherwise nothing to grab from adjacent pixels.
		float scale = dstDim/(float)srcDim;
		if (scale <= 1.0f)
			scale = 1.0f;
		scale = 1.0f/scale;

		float halfPixel = 0.5f/dstDim;
		float srcHalfPixel = 0.5f/srcDim;

		for (int i = 0; i < dstDim; i++)
		{
			float dstCenter = i / (float)dstDim + halfPixel;
			int closestSource = static_cast<int>(round((dstCenter - srcHalfPixel) * srcDim));
			
			auto& entry = FilterCache[i];
			entry.left = std::max(closestSource - kernel_width, 0);
			entry.right = std::min(closestSource + kernel_width, srcDim-1);

			float totalWeight = 0;
			for (int j = entry.left; j <= entry.right; ++j)
			{
				float loc = (j / (float)srcDim) + srcHalfPixel;
				float filterLoc = (loc - dstCenter)*dstDim*scale;
				float weight = value(filterLoc);
				if (abs(weight) < std::numeric_limits<float>::epsilon() && entry.cache.empty()) //strip any if we don't have a real weight yet.
				{
					entry.left++;
				}
				else
				{
					totalWeight += weight;
					entry.cache.push_back(weight);
				}
			}
			for (int j = entry.right; j >= entry.left; --j)
			{
				if (abs(entry.cache[j - entry.left]) < std::numeric_limits<float>::epsilon())
				{
					--entry.right;
					entry.cache.pop_back();
				}
				else
				{
					break;
				}
			}

			for (int j = entry.left; j <= entry.right; ++j)
			{
				entry.cache[j - entry.left] /= totalWeight;
			}
		}
	}
};

template<typename FILTER, size_t ELEMENT_SIZE>
void resize_horizontal(const info_t& src_info, const const_mapped_subresource& src, const info_t& dst_info, const mapped_subresource& dst)
{
	if (src_info.dimensions.y != dst_info.dimensions.y)
		throw std::invalid_argument("Horizontal resize assumes y dimensions are the same for source and destination");

	FILTER filter;
	filter.initialize_filter(src_info.dimensions.x, dst_info.dimensions.x);

	const char* srcData = (char*)src.data;
	char* dstData = (char*)dst.data;

	for (uint32_t y = 0; y < dst_info.dimensions.y; y++)
	{
		const uint8_t* oRESTRICT srcRow = (uint8_t*)byte_add(srcData, y*src.row_pitch);
		uint8_t* oRESTRICT dstRow = (uint8_t*)byte_add(dstData, y*dst.row_pitch);

		for (uint32_t x = 0; x < dst_info.dimensions.x; x++)
		{
			auto& filterEntry = filter.FilterCache[x];
			std::array<float, ELEMENT_SIZE> result;
			result.fill(0.0f);

			for (int srcX = filterEntry.left; srcX <= filterEntry.right; srcX++)
			{
				for (size_t i = 0; i < ELEMENT_SIZE; i++)
				{
					float src = srcRow[srcX*ELEMENT_SIZE + i];
					src *= filterEntry.cache[srcX - filterEntry.left];
					result[i] += src;
				}
			}
			
			for (size_t i = 0; i < ELEMENT_SIZE; i++)
				dstRow[x*ELEMENT_SIZE + i] = static_cast<uint8_t>(clamp(result[i], 0.0f, 255.0f));
		}
	}
}

template<typename FILTER, int ELEMENT_SIZE>
void resize_vertical(const info_t& src_info, const const_mapped_subresource& src, const info_t& dst_info, const mapped_subresource& dst)
{
	if (src_info.dimensions.x != dst_info.dimensions.x)
		throw std::invalid_argument("vertical resize assumes x dimensions are the same for source and destination");

	FILTER filter;
	filter.initialize_filter(src_info.dimensions.y, dst_info.dimensions.y);

	const uint8_t* oRESTRICT srcData = (uint8_t*)src.data;
	uint8_t* oRESTRICT dstData = (uint8_t*)dst.data;

	for (uint32_t y = 0; y < dst_info.dimensions.y; y++)
	{
		uint8_t* oRESTRICT dstRow = byte_add(dstData, y*dst.row_pitch);

		for (uint32_t x = 0; x < dst_info.dimensions.x; x++)
		{
			auto& filterEntry = filter.FilterCache[y];
			std::array<float, ELEMENT_SIZE> result;
			result.fill(0.0f);

			for (int srcY = filterEntry.left;srcY <= filterEntry.right; ++srcY)
			{
				const uint8_t* oRESTRICT srcElement = byte_add(srcData, srcY*src.row_pitch + x*ELEMENT_SIZE);

				for (size_t i = 0;i < ELEMENT_SIZE; i++)
				{
					float src = srcElement[i];
					src *= filterEntry.cache[srcY - filterEntry.left];
					result[i] += src;
				}
			}
			for (size_t i = 0;i < ELEMENT_SIZE; i++)
			{
				dstRow[x*ELEMENT_SIZE + i] = static_cast<uint8_t>(clamp(result[i], 0.0f, 255.0f));
			}
		}
	}
}

template<typename FILTER, int ELEMENT_SIZE>
void resize_internal(const info_t& src_info, const const_mapped_subresource& src, const info_t& dst_info, const mapped_subresource& dst)
{
	// Assuming all our filters are separable for now.

	if (all(src_info.dimensions == dst_info.dimensions)) // no actual resize, just copy
	{
		memcpy2d(dst.data, dst.row_pitch, src.data, src.row_pitch, dst_info.dimensions.x*element_size(dst_info.format), dst_info.dimensions.y);
	}

	else if (FILTER::support == 1) // point sampling
	{		
		const char* srcData = (char*)src.data;
		char* dstData = (char*)dst.data;

		// Bresenham style for x
		int fixed_step = (src_info.dimensions.x / dst_info.dimensions.x)*ELEMENT_SIZE;
		int remainder = (src_info.dimensions.x % dst_info.dimensions.x);

		for (uint32_t y = 0; y < dst_info.dimensions.y; y++)
		{
			int row = (y * src_info.dimensions.y) / dst_info.dimensions.y;
			const char* oRESTRICT srcRow = byte_add(srcData, row * src.row_pitch);
			char* oRESTRICT dstRow = byte_add(dstData, y * dst.row_pitch);

			uint32_t step = 0;
			for (uint32_t x = 0; x < dst_info.dimensions.x; ++x)
			{
				for (size_t i = 0; i < ELEMENT_SIZE; i++)
					*dstRow++ = *(srcRow + i);
				srcRow += fixed_step;
				step += remainder;
				if (step >= dst_info.dimensions.x)
				{
					srcRow += ELEMENT_SIZE;
					step -= dst_info.dimensions.x;
				}
			}
		}
	}
	else // have to run a real filter
	{
		if (src_info.dimensions.x == dst_info.dimensions.x) // Only need a y filter
			resize_vertical<FILTER, ELEMENT_SIZE>(src_info, src, dst_info, dst);
		else if (src_info.dimensions.y == dst_info.dimensions.y) // Only need a x filter
			resize_horizontal<FILTER, ELEMENT_SIZE>(src_info, src, dst_info, dst);
		else if (dst_info.dimensions.x*src_info.dimensions.y <= dst_info.dimensions.y*src_info.dimensions.x) // more efficient to run x filter then y
		{
			info_t tempInfo = src_info;
			tempInfo.dimensions.x = dst_info.dimensions.x;
			tempInfo.mip_layout = mip_layout::tight;
			std::vector<char> tempImage;
			tempImage.resize(total_size(tempInfo));

			mapped_subresource tempMap = map_subresource(tempInfo, 0, tempImage.data());
			resize_horizontal<FILTER, ELEMENT_SIZE>(src_info, src, tempInfo, tempMap);
			const_mapped_subresource tempMapConst = tempMap;
			resize_vertical<FILTER, ELEMENT_SIZE>(tempInfo, tempMapConst, dst_info, dst);
		}
		else // more efficient to run y filter then x
		{
			info_t tempInfo = src_info;
			tempInfo.dimensions.y = dst_info.dimensions.y;
			tempInfo.mip_layout = mip_layout::tight;
			std::vector<char> tempImage; // todo: fix internal allocation
			tempImage.resize(total_size(tempInfo));

			mapped_subresource tempMap = map_subresource(tempInfo, 0, tempImage.data());
			resize_vertical<FILTER, ELEMENT_SIZE>(src_info, src, tempInfo, tempMap);
			const_mapped_subresource tempMapConst = tempMap;
			resize_horizontal<FILTER, ELEMENT_SIZE>(tempInfo, tempMapConst, dst_info, dst);
		}
	}
}

void resize(const info_t& src_info, const const_mapped_subresource& src, const info_t& dst_info, const mapped_subresource& dst, const filter& f)
{
	oSURF_CHECK(src_info.mip_layout == dst_info.mip_layout && src_info.format == dst_info.format, "incompatible surfaces");
	oSURF_CHECK(!is_block_compressed(src_info.format), "block compressed formats cannot be padded");

	const int elementSize = element_size(src_info.format);

	#define FILTER_CASE(filter_type) \
	case filter::filter_type: \
	{	switch (elementSize) \
		{	case 1: resize_internal<filter_t<filter_##filter_type>,1>(src_info, src, dst_info, dst); break; \
			case 2: resize_internal<filter_t<filter_##filter_type>,2>(src_info, src, dst_info, dst); break; \
			case 3: resize_internal<filter_t<filter_##filter_type>,3>(src_info, src, dst_info, dst); break; \
			case 4: resize_internal<filter_t<filter_##filter_type>,4>(src_info, src, dst_info, dst); break; \
		} \
		break; \
	}

	switch (f)
	{
		FILTER_CASE(point)
		FILTER_CASE(box)
		FILTER_CASE(triangle)
		FILTER_CASE(lanczos2)
		FILTER_CASE(lanczos3)
		default: throw std::invalid_argument("unsupported filter type");
	}

	#undef FILTER_CASE
}

void clip(const info_t& src_info, const const_mapped_subresource& src, const info_t& dst_info, const mapped_subresource& dst, uint2 src_offset)
{
	oSURF_CHECK(src_info.mip_layout == dst_info.mip_layout && src_info.format == dst_info.format, "incompatible surfaces");
	oSURF_CHECK(!is_block_compressed(src_info.format), "block compressed formats cannot be padded");
	oSURF_CHECK(all(src_offset >= int2(0,0)), "source offset must be >= 0");
	auto bottom_right = src_offset + dst_info.dimensions.xy();
	oSURF_CHECK(all(bottom_right <= src_info.dimensions.xy()), "src_offset + the dimensions of the destination, must be within the dimensions of the source");
	auto elementSize = ouro::surface::element_size(src_info.format);
	memcpy2d(dst.data, dst.row_pitch, byte_add(src.data, src.row_pitch*src_offset.y + elementSize*src_offset.x), src.row_pitch, dst_info.dimensions.x*elementSize, dst_info.dimensions.y);
}

void pad(const info_t& src_info, const const_mapped_subresource& src, const info_t& dst_info, const mapped_subresource& dst, uint2 dst_offset)
{
	oSURF_CHECK(src_info.mip_layout == dst_info.mip_layout && src_info.format == dst_info.format, "incompatible surfaces");
	oSURF_CHECK(!is_block_compressed(src_info.format), "block compressed formats cannot be padded");
	oSURF_CHECK(all(dst_offset >= int2(0,0)), "destination offset must be >= 0");
	int2 bottom_right = dst_offset + src_info.dimensions.xy();
	oSURF_CHECK(all(bottom_right <= dst_info.dimensions.xy()), "dst_offset + destination dimensions must be within the dimensions of the source");
	int elementSize = element_size(src_info.format);
	memcpy2d(byte_add(dst.data, dst.row_pitch*dst_offset.y + elementSize*dst_offset.x), dst.row_pitch, src.data, src.row_pitch, src_info.dimensions.x*elementSize, src_info.dimensions.y);
}

}}
