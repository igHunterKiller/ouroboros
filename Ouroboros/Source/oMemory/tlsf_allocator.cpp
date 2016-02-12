// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/tlsf_allocator.h>
#include <oCore/byte.h>
#include <tlsf/tlsf.h>

#define USE_ALLOCATOR_STATS 1
#define USE_ALLOCATION_STATS 1
#define USE_LABEL 1

namespace ouro {

// How do I walk all pools?
static void o_tlsf_walk_heap(tlsf_t tlsf, tlsf_walker walker, void* user)
{
	tlsf_walk_pool(tlsf_get_pool(tlsf), walker, user);
}

struct walk_stats
{
	size_t largest_free_block_bytes;
	size_t num_free_blocks;
};

static void find_largest_free_block(void* ptr, size_t bytes, int used, void* user)
{
	if (!used)
	{
		walk_stats* stats_ = (walk_stats*)user;
		stats_->largest_free_block_bytes = std::max(stats_->largest_free_block_bytes, bytes);
		stats_->num_free_blocks++;
	}
}

void tlsf_allocator::initialize(void* arena, size_t bytes)
{
	if (!arena)
		throw allocate_error(allocate_errc::invalid_bookkeeping);

  if (!aligned(arena, 16))
		throw allocate_error(allocate_errc::alignment);

	heap_ = arena;
	heap_size_ = bytes;
	reset();
}

void* tlsf_allocator::deinitialize()
{
	void* arena = heap_;
  if (arena)
  {
	  #if USE_ALLOCATOR_STATS
		  if (stats_.num_allocations)
			  throw allocate_error(allocate_errc::outstanding_allocations);
	  #endif

	  if (!valid())
		  throw allocate_error(allocate_errc::corrupt);

	  tlsf_destroy(heap_);
	  heap_ = nullptr;
  }
	return arena;
}

tlsf_allocator::tlsf_allocator(tlsf_allocator&& that)
{
	operator=(std::move(that));
}

tlsf_allocator& tlsf_allocator::operator=(tlsf_allocator&& that)
{
	if (this != &that)
	{
		deinitialize();
		heap_ = that.heap_; that.heap_ = nullptr;
		heap_size_ = that.heap_size_; that.heap_size_ = 0;
		stats_ = that.stats_; that.stats_ = allocator_stats();
	}

	return *this;
}

allocator_stats tlsf_allocator::get_stats() const
{
	allocator_stats s = stats_;
	walk_stats ws;
	o_tlsf_walk_heap(heap_, find_largest_free_block, &ws);
	s.largest_free_block_bytes = ws.largest_free_block_bytes;
	s.num_free_blocks = ws.num_free_blocks;
	return s;
}

void* tlsf_allocator::allocate(size_t bytes, const char* label, const allocate_options& options)
{
	bytes = std::max(bytes, size_t(1));

	#if USE_LABEL
		// tlsf uses a pointer at the end of a free block. When allocated, overwrite
		// that location with a label for debugging memory stomps.
		bytes += sizeof(const char*);
	#endif

	size_t align = options.convert_alignment();
	void* p = align == 16 ? tlsf_malloc(heap_, bytes) : tlsf_memalign(heap_, align, bytes);
	if (p)
	{
		size_t block_size = tlsf_block_size(p);
		#if USE_ALLOCATOR_STATS
			stats_.num_allocations++;
			stats_.num_allocations_peak = std::max(stats_.num_allocations_peak, stats_.num_allocations);
			stats_.allocated_bytes += block_size;
			stats_.allocated_bytes_peak = std::max(stats_.allocated_bytes_peak, stats_.allocated_bytes);
		#endif

		#if USE_LABEL
			const char** label_dst = (const char**)((char*)p + block_size - sizeof(const char*));
			*label_dst = label;
		#endif
	}

	#if USE_ALLOCATION_STATS
		allocation_stats s;
		s.pointer = p;
		s.label = label;
		s.size = bytes;
		s.options = options;
		s.ordinal = 0;
		s.frame = 0;
		s.operation = memory_operation::allocate;
		default_allocate_track(0, s);
	#endif

	return p;
}

void* tlsf_allocator::reallocate(void* ptr, size_t bytes)
{
	size_t block_size = ptr ? tlsf_block_size(ptr) : 0;
	void* p = tlsf_realloc(heap_, ptr, bytes);
	if (p)
	{
		size_t diff = tlsf_block_size(p) - block_size;
		#if USE_ALLOCATOR_STATS
			stats_.allocated_bytes += diff;
			stats_.allocated_bytes_peak = std::max(stats_.allocated_bytes_peak, stats_.allocated_bytes);
		#endif
	}

	#if USE_ALLOCATION_STATS
		allocation_stats s;
		s.pointer = p;
		s.label = nullptr;
		s.size = bytes;
		s.options = allocate_options();
		s.ordinal = 0;
		s.frame = 0;
		s.operation = memory_operation::reallocate;
		default_allocate_track(0, s);
	#endif

	return p;
}

void tlsf_allocator::deallocate(void* ptr)
{
	if (ptr)
	{
		const size_t block_size = tlsf_block_size(ptr);
		tlsf_free(heap_, ptr);

		#if USE_ALLOCATOR_STATS
			stats_.num_allocations--;
			stats_.allocated_bytes -= block_size;
		#endif
	}

	#if USE_ALLOCATION_STATS
		allocation_stats s;
		s.pointer = ptr;
		s.label = nullptr;
		s.size = 0;
		s.options = allocate_options();
		s.ordinal = 0;
		s.frame = 0;
		s.operation = memory_operation::deallocate;
		default_allocate_track(0, s);
	#endif
}

size_t tlsf_allocator::size(void* ptr) const 
{
	return tlsf_block_size(ptr);
}

bool tlsf_allocator::owns(void* ptr) const
{
	return ptr >= heap_ && ptr < ((uint8_t*)heap_ + heap_size_);
}

bool tlsf_allocator::valid() const
{
	return heap_ && !tlsf_check(heap_);
}

void tlsf_allocator::reset()
{
	if (!heap_ || !heap_size_)
		throw allocate_error(allocate_errc::corrupt);
	heap_ = tlsf_create_with_pool(heap_, heap_size_);

	#if USE_ALLOCATOR_STATS
		stats_ = allocator_stats();
		stats_.capacity_bytes = heap_size_ - tlsf_size();
	#endif
}

void tlsf_allocator::walk_heap(allocate_heap_walk_fn walker, void* user)
{
	if (walker)
		o_tlsf_walk_heap(heap_, walker, user);
}

}
