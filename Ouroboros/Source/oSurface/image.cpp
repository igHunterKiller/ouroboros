// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSurface/image.h>
#include <oSurface/algo.h>
#include <oSurface/box.h>
#include <oSurface/convert.h>
#include <oMemory/memory.h>
#include <mutex>

namespace ouro { namespace surface {

image::image(image&& that) 
	: info_(that.info_)
	, bits_(that.bits_)
	, alloc_(that.alloc_)
{
	that.bits_ = nullptr;
	that.info_ = info();
	that.alloc_ = allocator();
}

image& image::operator=(image&& that)
{
	if (this != &that)
	{
		mtx.lock();
		that.mtx.lock();
		deinitialize();
		bits_ = that.bits_; that.bits_ = nullptr; 
		info_ = that.info_; that.info_ = info();
		alloc_ = that.alloc_; that.alloc_ = allocator();
		that.mtx.unlock();
		mtx.unlock();
	}
	return *this;
}

void image::initialize(const info_t& info, const allocator& alloc)
{
	deinitialize();
	info_ = info;
	bits_ = alloc.allocate(size(), "image");
	alloc_ = alloc;
}

void image::initialize(const info_t& info, const void* data, const allocator& alloc)
{
	deinitialize();
	info_ = info;
	bits_ = (void*)data;
	alloc_ = alloc;
}

void image::initialize_array(const image* const* sources, uint32_t num_sources, bool mips)
{
	deinitialize();
	info_t si = sources[0]->info();
	oCheck(si.mip_layout == mip_layout::none, std::errc::invalid_argument, "all images in the specified array must be simple types and the same 2D dimensions");
	oCheck(si.dimensions.z == 1, std::errc::invalid_argument, "all images in the specified array must be simple types and the same 2D dimensions");
	si.mip_layout = mips ? mip_layout::tight : mip_layout::none;
	si.array_size = static_cast<int>(num_sources);
	initialize(si);

	const uint32_t nMips = num_mips(mips, si.dimensions);
	const uint32_t nSlices = si.safe_array_size();
	for (uint32_t i = 0; i < nSlices; i++)
	{
		int dst = calc_subresource(0, i, 0, nMips, nSlices);
		int src = calc_subresource(0, 0, 0, nMips, 0);
		copy_from(dst, *sources[i], src);
	}

	if (mips)
		generate_mips();
}

void image::initialize_3d(const image* const* sources, uint32_t num_sources, bool mips)
{
	deinitialize();
	info_t si = sources[0]->info();
	oCheck(si.mip_layout == mip_layout::none, std::errc::invalid_argument, "all images in the specified array must be simple types and the same 2D dimensions");
	oCheck(si.dimensions.z == 1,              std::errc::invalid_argument, "all images in the specified array must be simple types and the same 2D dimensions");
	oCheck(si.array_size == 0,                std::errc::invalid_argument, "arrays of 3d surfaces not yet supported");
	si.mip_layout = mips ? mip_layout::tight : mip_layout::none;
	si.dimensions.z = static_cast<int>(num_sources);
	si.array_size = 0;
	initialize(si);

	box_t box;
	box.right = si.dimensions.x;
	box.bottom = si.dimensions.y;
	for (uint32_t i = 0; i < si.dimensions.z; i++)
	{
		box.front = i;
		box.back = i + 1;
		shared_lock lock(sources[i]);
		update_subresource(0, box, lock.mapped);
	}

	if (mips)
		generate_mips();
}

void image::deinitialize()
{
	if (bits_ && alloc_)
		alloc_.deallocate(bits_);
	bits_ = nullptr;
	alloc_ = allocator();
}

void image::clear()
{
	lock_t lock(mtx);
	memset(bits_, 0, size());
}

void image::fill(uint32_t argb)
{
  lock_t lock(mtx);

  const uint32_t nsubresources = num_subresources(info_);
  for (uint32_t subresource = 0; subresource < nsubresources; subresource++)
  {
    uint2 byte_dimensions;
    mapped_subresource mapped = map_subresource(info_, subresource, bits_, &byte_dimensions);

    switch (info_.format)
    {
      case surface::format::r8g8_unorm:
      {
        const uint16_t rg = (argb >> 8) & 0xfff;
        memset2d2(mapped.data, mapped.row_pitch, rg, byte_dimensions.x, byte_dimensions.y);
        break;
      }

			// @tony: these aren't the same thing, but until the renderer gets a little more linear, leave these as the same for now
      case surface::format::b8g8r8a8_unorm:
      case surface::format::b8g8r8a8_unorm_srgb:
      {
        memset2d4(mapped.data, mapped.row_pitch, argb, byte_dimensions.x, byte_dimensions.y);
        break;
      }

      default:
        oThrow(std::errc::invalid_argument, "unsupported default texture format");
    }
  }
}

void image::flatten()
{
	oCheck(!is_block_compressed(info_.format), std::errc::not_supported, "block compressed formats not handled yet");
	int rp = row_pitch(info_);
	size_t sz = size();
	info_.mip_layout = mip_layout::none;
	info_.dimensions = int3(rp / element_size(info_.format), int(sz / rp), 1);
	info_.array_size = 0;
}

void image::update_subresource(uint32_t subresource, const const_mapped_subresource& src, const copy_option& option)
{
	uint2 bd;
	mapped_subresource dst = map_subresource(info_, subresource, bits_, &bd);
	lock_t lock(mtx);
	memcpy2d(dst.data, dst.row_pitch, src.data, src.row_pitch, bd.x, bd.y, option == copy_option::flip_vertically);
}

void image::update_subresource(uint32_t subresource, const box_t& box, const const_mapped_subresource& src, const copy_option& option)
{
	if (is_block_compressed(info_.format) || info_.format == format::r1_unorm)
		oThrow(std::errc::invalid_argument, "block compressed and bit formats not supported");

	uint2 bd;
	mapped_subresource Dest = map_subresource(info_, subresource, bits_, &bd);

	const auto num_rows = box.height();
	auto pixel_size = element_size(info_.format);
	auto row_size = pixel_size * box.width();

	// Dest points at start of subresource, so offset to subrect of first slice
	Dest.data = byte_add(Dest.data, box.front * Dest.depth_pitch + box.top * Dest.row_pitch + box.left * pixel_size);

	const void* pSource = src.data;

	lock_t lock(mtx);
	for (uint32_t slice = box.front; slice < box.back; slice++)
	{
		memcpy2d(Dest.data, Dest.row_pitch, pSource, src.row_pitch, row_size, num_rows, option == copy_option::flip_vertically);
		Dest.data = byte_add(Dest.data, Dest.depth_pitch);
		pSource = byte_add(pSource, src.depth_pitch);
	}
}

void image::map(uint32_t subresource, mapped_subresource* out_mapped, uint2* out_byte_dimensions)
{
	mtx.lock();
	try
	{
		*out_mapped = map_subresource(info_, subresource, bits_, out_byte_dimensions);
	}

	catch (std::exception&)
	{
		auto e = std::current_exception();
		mtx.unlock();
		std::rethrow_exception(e);
	}
}

void image::unmap(uint32_t subresource)
{
	mtx.unlock();
}

void image::map_const(uint32_t subresource, const_mapped_subresource* out_mapped, uint2* out_byte_dimensions) const
{
	lock_shared();
	*out_mapped = map_const_subresource(info_, subresource, bits_, out_byte_dimensions);
}

void image::unmap_const(uint32_t subresource) const
{
	unlock_shared();
}

void image::copy_to(uint32_t subresource, const mapped_subresource& dst, const copy_option& option) const
{
	uint2 bd;
	const_mapped_subresource src = map_const_subresource(info_, subresource, bits_, &bd);
	lock_shared_t lock(mtx);
	memcpy2d(dst.data, dst.row_pitch, src.data, src.row_pitch, bd.x, bd.y, option == copy_option::flip_vertically);
}

image image::convert(const info_t& dst_info) const
{
	return convert(dst_info, alloc_);
}

image image::convert(const info_t& dst_info, const allocator& alloc) const
{
	if (any(dst_info.dimensions != info_.dimensions))
		oThrow(std::errc::invalid_argument, "dimensions mismatch, implement a resize here");

	image converted(dst_info, alloc);
	shared_lock slock(this);
	lock_guard dlock(converted);

	const auto nsubresources = num_subresources(info_);
	for (uint32_t subresource = 0; subresource < nsubresources; subresource++)
	{
		auto subinfo = subresourceinfo(info_, subresource);
		auto mapped_src = map_const_subresource(info_, subresource, slock.mapped.data);
		auto mapped_dst = map_subresource(dst_info, subresource, dlock.mapped.data);

		convert_formatted(mapped_dst, dst_info.format, mapped_src, info_.format, subinfo.dimensions);
	}

	return converted;
}

void image::convert_to(uint32_t subresource, const mapped_subresource& dst, const format& dst_format, const copy_option& option) const
{
	if (info_.format == dst_format)
		copy_to(subresource, dst, option);
	else
	{
		shared_lock slock(this, subresource);
		auto subinfo = subresourceinfo(info_, subresource);
		convert_formatted(dst, dst_format, slock.mapped, info_.format, subinfo.dimensions, option);
	}
}

void image::convert_from(uint32_t subresource, const const_mapped_subresource& src, const format& src_format, const copy_option& option)
{
	if (info_.format == src_format)
		copy_from(subresource, src, option);
	else
	{
		info_t src_info = info_;
		src_info.format = src_format;
		subresource_info_t subinfo = subresourceinfo(src_info, subresource);
		lock_guard lock(this);
		convert_formatted(lock.mapped, info_.format, src, src_format, subinfo.dimensions, option);
	}
}

void image::convert_in_place(const format& fmt)
{
	lock_guard lock(this);
	swizzle_formatted(info_, fmt, lock.mapped);
	info_.format = fmt;
}

void image::generate_mips(const filter& f)
{
	lock_t lock(mtx);

	uint32_t nMips = num_mips(info_);
	uint32_t nSlices = info_.safe_array_size();

	for (uint32_t slice = 0; slice < nSlices; slice++)
	{
		int mip0subresource = calc_subresource(0, slice, 0, nMips, info_.array_size);
		const_mapped_subresource mip0 = map_const_subresource(info_, mip0subresource, bits_);

		for (uint32_t mip = 1; mip < nMips; mip++)
		{
			uint32_t subresource = calc_subresource(mip, slice, 0, nMips, info_.array_size);
			auto subinfo = subresourceinfo(info_, subresource);

			mapped_subresource dst = map_subresource(info_, subresource, bits_);

			uint32_t nDepthSlices = max(1u, subinfo.dimensions.z);
			for (uint32_t depth = 0; depth < nDepthSlices; depth++)
			{
				info_t di = info_;
				di.dimensions = subinfo.dimensions;
				resize(info_, mip0, di, dst, f);
				dst.data = byte_add(dst.data, dst.depth_pitch);
			}
		}
	}
}

float calc_rms(const image& b1, const image& b2)
{
	return calc_rms(b1, b2, nullptr);
}

float calc_rms(const image& b1, const image& b2, image* out_diffs, int diff_scale, const allocator& alloc)
{
	info_t si1 = b1.info();
	info_t si2 = b2.info();

	if (any(si1.dimensions != si2.dimensions)) oThrow(std::errc::invalid_argument, "mismatched dimensions");
	if (si1.format != si2.format) oThrow(std::errc::invalid_argument, "mismatched format");
	if (si1.array_size != si2.array_size) oThrow(std::errc::invalid_argument, "mismatched array_size");
	int n1 = num_subresources(si1);
	int n2 = num_subresources(si2);
	if (n1 != n2) oThrow(std::errc::invalid_argument, "incompatible layouts");

	info_t dsi;
	if (out_diffs)
	{
		dsi = si1;
		dsi.format = format::r8_unorm;
		out_diffs->initialize(dsi);
	}

	float rms = 0.0f;
	for (int i = 0; i < n1; i++)
	{
		mapped_subresource msr;
		if (out_diffs)
			out_diffs->map(i, &msr);

		shared_lock lock1(b1, i);
		shared_lock lock2(b2, i);
	
		if (out_diffs)
			rms += calc_rms(si1, lock1.mapped, lock2.mapped, dsi, msr);
		else
			rms += calc_rms(si1, lock1.mapped, lock2.mapped);

		if (out_diffs)
			out_diffs->unmap(i);
	}

	return rms / static_cast<float>(n1);
}

}}
