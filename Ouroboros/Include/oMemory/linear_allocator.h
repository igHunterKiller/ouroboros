// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) linear allocator: cannot free but is extremely quick to allocate

#pragma once
#include <oArch/compiler.h>
#include <oCore/byte.h>
#include <cstdint>

namespace ouro {

class linear_allocator
{
public:
	static const size_t default_alignment = oDEFAULT_MEMORY_ALIGNMENT;

	// ctor creates as empty
	linear_allocator() : base_(nullptr), end_(nullptr), head_(nullptr) {}

	// ctor moves an existing pool into this one
	linear_allocator(linear_allocator&& that)
		: base_(that.base_), end_(that.end_), head_(that.head_)
	{ that = linear_allocator(); }

	// ctor creates as a valid pool using external memory
	linear_allocator(void* arena, size_t bytes) { initialize(arena, bytes); }

	// dtor
	~linear_allocator() { deinitialize(); }

	// calls deinitialize on this, moves that's memory under the same config
	linear_allocator& operator=(linear_allocator&& that)
	{
		if (this != &that)
		{
			deinitialize();
			base_ = that.base_; that.base_ = nullptr;
			end_ = that.end_; that.end_ = nullptr;
			head_ = that.head_; that.head_ = nullptr;
		}
		return *this;
	}
	
	// returns bytes required for arena; pass nullptr to obtain size, allocate
	// and then pass that to memory in a second call to initialize the class.
	size_t initialize(void* arena, size_t bytes)
	{
		if (arena)
		{
			base_ = (uint8_t*)arena;
			end_ = base_ + bytes;
			head_ = base_;
		}

		return bytes;
	}

	// deinitializes the pool and returns the memory passed to initialize()
	void* deinitialize()
	{
		void* p = base_;
		base_ = end_ = head_ = nullptr;
		return p;
	}

	// returns true if this class has been initialized
	inline bool valid() const { return !!base_; }

	// returns the number of bytes available
	inline size_t size_free() const { return size_t(end_ - head_); }

	// returns the number of allocated bytes
	inline size_t size() const { return size_t(head_ - base_); }

	// returns true of there are no outstanding allocations
	inline bool full() const { return capacity() == size_free(); }

	// returns the max number of bytes that can be allocated
	inline size_t capacity() const { return size_t(end_ - base_); }

	// returns true if all bytes have been allocated
	inline bool empty() const { return head_ >= end_; }

	// access underlying memory: for use when this allocator 
	// accumulates a buffer that is then use directly.
	const void* base() const { return base_; }

	void* allocate(size_t bytes, size_t alignment = default_alignment)
	{ 
		uint8_t* p = align(head_, alignment);
		uint8_t* h = p + bytes;
		if (h <= end_)
		{
			head_ = h;
			return p;
		}
		return nullptr;
	}

	template<typename T> T* allocate(size_t size = sizeof(T), size_t alignment = default_alignment) { return (T*)allocate(size, alignment); }

	// reset the linear allocator to full availability
	void reset() { head_ = base_; }

	// simple range check that returns true if this index/pointer could have been allocated from this pool
	bool owns(void* ptr) const { return ptr >= base_ && ptr < end_; }

private:
	uint8_t* base_;
	uint8_t* end_;
	uint8_t* head_;
};

}
