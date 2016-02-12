// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Uses surface utility functions to manage a buffer filled with all that a 
// surface can support. This is basically a CPU-side version of similar GPU 
// buffers in D3D and OGL.

#pragma once
#include <oMemory/allocate.h>
#include <oConcurrency/mutex.h>
#include <oSurface/box.h>
#include <oSurface/surface.h>
#include <oSurface/resize.h>
#include <memory>

namespace ouro { namespace surface {
	
class image
{
public:
	image() : bits_(nullptr) {}
	image(const info_t& info, const allocator& alloc = default_allocator) { initialize(info, alloc); }
	image(const info_t& info, const void* data, const allocator& alloc = noop_allocator) { initialize(info, data, alloc); }

	~image() { deinitialize(); }

	image(image&& that);
	image& operator=(image&& that);

	// create a buffer with uninitialized bits
	void initialize(const info_t& info, const allocator& alloc = default_allocator);
	
	// create a buffer using the specified pointer; alloc will be used to manage its lifetime.
	void initialize(const info_t& info, const void* data, const allocator& alloc = noop_allocator);

	// create an array buffer out of several subbuffers of the same format
	void initialize_array(const image* const* sources, uint32_t num_sources, bool mips = false);
	template<size_t N> void initialize_array(const image* const (&sources)[N], bool mips = false) { initialize_array(sources, N, mips); }

	// creates a 3d surface out of several subbuffers of the same format
	void initialize_3d(const image* const* texel_sources, uint32_t num_sources, bool mips = false);
	template<size_t N> void initialize_3d(const image* const (&sources)[N], bool mips = false) { initialize_3d(sources, N, mips); }

	void deinitialize();

	operator bool() const { return !!bits_; }
	inline bool immutable() const { return !!bits_ && !alloc_; }

	inline info_t info() const { return info_; }
	inline void set_semantic(const semantic& s) { info_.semantic = s; }

	// returns the size of the bit data: all subresources and padding, not including the info
	inline size_t size() const { return total_size(info()); }

	// Sets all memory 0, including padding memory
	void clear();

  // sets all subresources to a close interpretation of the specified color (this will 
  // swizzle/truncate/cast channels).
  void fill(uint32_t argb);

	// Without modifying the data this updates the info to be an image layout with 
	// array_size of 0. This is useful for saving the buffer to a files as the entire
	// surface is laid out.
	void flatten();

	// copies the specified src of the same format and dimensions into a subresource in the current instance
	void update_subresource(uint32_t subresource, const const_mapped_subresource& src, const copy_option& option = copy_option::none);
	void update_subresource(uint32_t subresource, const box_t& box, const const_mapped_subresource& src, const copy_option& option = copy_option::none);

	// locks internal memory for read/write and returns parameters for working with it
	void map(uint32_t subresource, mapped_subresource* out_mapped, uint2* out_byte_dimensions = nullptr);
	void unmap(uint32_t subresource);

	// locks internal memory for read-only and returns parameters for working with it
	void map_const(uint32_t subresource, const_mapped_subresource* out_mapped, uint2* out_byte_dimensions = nullptr) const;
	void unmap_const(uint32_t subresource) const;

	// copies from a subresource in this instance to a mapped destination of the same format and dimensions 
	void copy_to(uint32_t subresource, const mapped_subresource& dst, const copy_option& option = copy_option::none) const;
	inline void copy_from(uint32_t subresource, const const_mapped_subresource& src, const copy_option& option = copy_option::none) { update_subresource(subresource, src, option); }
	inline void copy_from(uint32_t subresource, const image& src, uint32_t src_subresource, const copy_option& option = copy_option::none);

	// initializes a resized and reformatted copy of this buffer allocated from the same or a user-specified allocator
	image convert(const info_t& dst_info) const;
	image convert(const info_t& dst_info, const allocator& alloc) const;

	// initializes a reformatted copy of this buffer allocated from the same or a user-specified allocator
	inline image convert(const format& dst_format) const { info_t si = info(); si.format = dst_format; return convert(si); }
	inline image convert(const format& dst_format, const allocator& alloc) const { info_t si = info(); si.format = dst_format; return convert(si, alloc); }

	// copies to a mapped subresource of the same dimension but the specified format
	void convert_to(uint32_t subresource, const mapped_subresource& dst, const format& dst_format, const copy_option& option = copy_option::none) const;

	// copies into this instance from a source of the same dimension but a different format
	void convert_from(uint32_t subresource, const const_mapped_subresource& src, const format& src_format, const copy_option& option = copy_option::none);

	// For compatible types such as RGB <-> BGR do conversion in-place
	void convert_in_place(const format& fmt);

	// Uses the top-level mip as a source and replaces all other mips with a filtered version
	void generate_mips(const filter& f = filter::lanczos2);

private:
	info_t info_;
	void* bits_;
	allocator alloc_;
	
	typedef ouro::shared_mutex mutex_t;
	typedef ouro::lock_guard<mutex_t> lock_t;
	typedef ouro::shared_lock<mutex_t> lock_shared_t;
	
	mutable mutex_t mtx;
	inline void lock_shared() const { mtx.lock_shared(); }
	inline void unlock_shared() const { mtx.unlock_shared(); }

	image(const image&);
	const image& operator=(const image&);
};

class lock_guard
{
public:
	lock_guard(image& b, uint32_t subresource = 0)
		: buf(&b)
		, subresource(subresource)
	{ buf->map(subresource, &mapped, &byte_dimensions); }

	lock_guard(image* b, uint32_t subresource = 0)
		: buf(b)
		, subresource(subresource)
	{ buf->map(subresource, &mapped, &byte_dimensions); }

	~lock_guard() { buf->unmap(subresource); }

	mapped_subresource mapped;
	uint2 byte_dimensions;

private:
	image* buf;
	uint32_t subresource;

	lock_guard(const lock_guard&);
	const lock_guard& operator=(const lock_guard&);
};

class shared_lock
{
public:
	shared_lock(image& b, uint32_t subresource = 0)
		: buf(&b)
		, subresource(subresource)
	{ buf->map_const(subresource, &mapped, &byte_dimensions); }

	shared_lock(image* b, uint32_t subresource = 0)
		: buf(b)
		, subresource(subresource)
	{ buf->map_const(subresource, &mapped, &byte_dimensions); }

	shared_lock(const image* b, uint32_t subresource = 0)
		: buf(b)
		, subresource(subresource)
	{ buf->map_const(subresource, &mapped, &byte_dimensions); }

	shared_lock(const image& b, uint32_t subresource = 0)
		: buf(&b)
		, subresource(subresource)
	{ buf->map_const(subresource, &mapped, &byte_dimensions); }

	~shared_lock() { buf->unmap_const(subresource); }

	const_mapped_subresource mapped;
	uint2 byte_dimensions;

private:
	uint32_t subresource;
	const image* buf;

	shared_lock(const shared_lock&);
	const shared_lock& operator=(const shared_lock&);
};

inline void image::copy_from(uint32_t subresource, const image& src, uint32_t src_subresource, const copy_option& option)
{
	shared_lock locked(src, src_subresource);
	copy_from(subresource, locked.mapped, option);
}

// returns the root mean square of the difference between the two surfaces. If
// the formats or sizes are different, this throws an exception. If out_diffs
// is passed in, it will be initialized using the specified allocator. The rms
// grayscale color will be multiplied by diff_scale.
float calc_rms(const image& b1, const image& b2);
float calc_rms(const image& b1, const image& b2, image* out_diffs, int diff_scale = 1, const allocator& alloc = default_allocator);

}}
