// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) fine-grained concurrent linear allocator: cannot free but is extremely 
// quick to allocate

#pragma once
#include <oArch/compiler.h>
#include <oCore/byte.h>
#include <atomic>
#include <cstdint>

namespace ouro {

class concurrent_linear_allocator
{
public:
	static const size_t default_alignment = oDEFAULT_MEMORY_ALIGNMENT;

	// ctor creates as empty
	concurrent_linear_allocator() : base_(nullptr), end_(nullptr) { head_.store(nullptr); }

	// ctor moves an existing pool into this one
	concurrent_linear_allocator(concurrent_linear_allocator&& that)
		: base_(that.base_), end_(that.end_)
	{ head_.store(that.head_.load()); that = concurrent_linear_allocator(); }

	// ctor creates as a valid pool using external memory
	concurrent_linear_allocator(void* arena, size_t bytes) { initialize(arena, bytes); }

	// dtor
	~concurrent_linear_allocator() { deinitialize(); }

	// calls deinitialize on this, moves that's memory under the same config
	concurrent_linear_allocator& operator=(concurrent_linear_allocator&& that)
	{
		if (this != &that)
		{
			deinitialize();
			base_ = that.base_; that.base_ = nullptr;
			end_ = that.end_; that.end_ = nullptr;
			head_.store(that.head_.load()); that.head_.store(nullptr);
		}
		return *this;
	}
	
	void initialize(void* arena, size_t bytes)
	{
		base_ = (uint8_t*)arena;
		end_ = base_ + bytes;
		head_ = base_;
	}

	// deinitializes the pool and returns the memory passed to initialize()
	void* deinitialize()
	{
		return base_;
	}

	// returns the number of bytes available
	inline size_t size_free() const { return size_t(end_ - head_.load()); }

	// returns the number of allocated bytes
	inline size_t size() const { return size_t(head_.load() - base_); }

	// returns true of there are no outstanding allocations
	inline bool full() const { return capacity() == size_free(); }

	// returns the max number of bytes that can be allocated
	inline size_t capacity() const { return size_t(end_ - base_); }

	// returns true if all bytes have been allocated
	inline bool empty() const { return head_.load() >= end_; }

	void* allocate(size_t bytes, size_t alignment = default_alignment)
	{
		uint8_t *n, *o, *p;
		do
		{
			o = head_;
			p = align(o, alignment);
			n = p + bytes;
			if (n >= end_)
				return nullptr;
		} while (!head_.compare_exchange_strong(o, n));
		return p;
	}

	template<typename T> T* allocate(size_t size = sizeof(T), size_t alignment = default_alignment) { return (T*)allocate(size, alignment); }

	// reset the linear allocator to full availability
	void reset() { head_.store(base_); }

	// simple range check that returns true if this index/pointer could have been allocated from this pool
	bool owns(void* ptr) const { return ptr >= base_ && ptr < end_; }

private:
	uint8_t* base_;
	uint8_t* end_;
	std::atomic<uint8_t*> head_;
};

}
