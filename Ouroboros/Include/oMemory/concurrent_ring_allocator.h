// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) fine-grained concurrent ring allocator: behaves like a linear 
// allocator but blocks on OOM. Memory is reclaimed by calling retire().

// Use case: allocating commands to a GPU. Once GPU is finished, call
// retire to notify that allocated memory is no longer in use.

#pragma once
#include <oArch/compiler.h>
#include <oCore/byte.h>
#include <atomic>
#include <cstdint>

namespace ouro {

class concurrent_ring_allocator
{
public:
	static const size_t default_alignment = oDEFAULT_MEMORY_ALIGNMENT;

	// === non-concurrent api ===

	// ctor creates as empty
	concurrent_ring_allocator() : base_(nullptr), end_(nullptr) { head_tail_ = 0; }

	// ctor moves an existing pool into this one
	concurrent_ring_allocator(concurrent_ring_allocator&& that)
		: base_(that.base_), end_(that.end_)
	{ head_tail_ = that.head_tail_; that = concurrent_ring_allocator(); }

	// ctor creates as a valid pool using external memory
	concurrent_ring_allocator(void* arena, size_t bytes) { initialize(arena, bytes); }

	// dtor
	~concurrent_ring_allocator() { deinitialize(); }

	// calls deinitialize on this, moves that's memory under the same config
	concurrent_ring_allocator& operator=(concurrent_ring_allocator&& that)
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
		head_tail_ = 0;
	}

	// deinitializes the pool and returns the memory passed to initialize()
	void* deinitialize()
	{
		return base_;
	}

	// returns the max number of bytes that can be allocated
	inline size_t capacity() const { return size_t(end_ - base_); }

	// returns the number of bytes available
	inline size_t size_free() const { HT o; o.head_tail = head_tail; return (o.head < o.tail) ? (capacity() - o.tail + o.head) : (o.tail - o.head); }

	// returns the number of allocated bytes
	inline size_t size() const { HT o; o.head_tail = head_tail; return (o.head < o.tail) ? (o.tail - o.head) : (capacity() - o.head + o.tail); }

	// returns true of there are no outstanding allocations
	inline bool full() const { return capacity() == size_free(); }

	// returns the allocator to an empty state
	void reset()
	{
		head_tail_ = 0;
	}

	// === concurrent api ===

	// simple range check that returns true if this index/pointer could have been allocated from this pool
	bool owns(void* ptr) const { return ptr >= base_ && ptr < end_; }

	// allocates memory of the specified size and alignment
	void* allocate(size_t bytes, size_t alignment)
	{
		uint8_t *p, *t;
		HT o, n;
		do
		{
			o.head_tail = head_tail;

			p = align(base + o.tail, alignment);
			n = p + bytes;

			if (o.head < o.tail)
			{
				if (n > end_)
				{
					// naive OOM, try wrapping
					p = align(base, alignment);
					n = p + bytes;

					if (n > o.head)
						return nullptr; // or spin waiting for a free
				}
			}

			else if (n > o.head)
				return nullptr; // or spin waiting for a free

			n.head = o.head;
			n.tail = uint32_t(n - base_);

		} while (!head_tail_.compare_exchange_strong(o.head_tail, n.head_tail));
		return p;
	}

	// returns the extent of allocates so far. Pass this value to reclaim() when
	// these allocates aren't required anymore.
	uint32_t marker()
	{
		return head_tail_.tail;
	}

	// like free() this makes memory available again up until the specified marker
	void reclaim(uint32_t marker)
	{
		HT o, n;
		do
		{
			o.head_tail = head_tail_;
			n.head = frame;
			n.tail = o.tail;

		} while(!head_tail_.compare_exchange_strong(o.head_tail, n.head_tail));
	}

private:
	concurrent_ring_allocator(concurrent_ring_allocator&);
	const concurrent_ring_allocator& operator=(const concurrent_ring_allocator&);

	union HT
	{
		uint64_t head_tail;
		struct
		{
			uint32_t head;
			uint32_t tail;
		};
	};

	uint8_t* base_;
	uint8_t* end_;
	std::atomic<uint64_t> head_tail_;
};

}
