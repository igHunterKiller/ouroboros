// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/sbb_allocator.h>
#include <oCore/bit.h>
#include <oMemory/sbb.h>
#include <algorithm>

#define USE_ALLOCATOR_STATS 1
#define USE_ALLOCATION_STATS 1

#include <oCore/assert.h>
#define SBB_TRACE(sbb_op, new_ptr, old_ptr, bytes, align, label) do { oTrace("[SBB], %s, %p, %p, %u, %u, %s", sbb_op, old_ptr, new_ptr, bytes, align, label); } while(false)
//#define SBB_TRACE(sbb_op, new_ptr, old_ptr, bytes, align, label) do {} while(false)


namespace ouro {

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

size_t sbb_allocator::calc_bookkeeping_size(size_t memory_bytes, size_t min_block_size)
{
  return sbb_bookkeeping_size(memory_bytes, min_block_size);
}

void sbb_allocator::initialize(void* memory, size_t memory_bytes, void* bookkeeping, size_t min_block_size)
{
  if (!bookkeeping)
    throw allocate_error(allocate_errc::invalid_bookkeeping);

	if (bookkeeping)
	{
		deinitialize();

		heap_ = bookkeeping;
		heap_size_ = memory_bytes;
		min_block_size_ = min_block_size;

		heap_ = sbb_create(memory, heap_size_, min_block_size, heap_);

		#if USE_ALLOCATOR_STATS
			stats_ = allocator_stats();
			stats_.capacity_bytes = heap_size_ - sbb_overhead((sbb_t)heap_);
		#endif
	}
}

void* sbb_allocator::deinitialize()
{
	void* memory = heap_;
  if (heap_)
  {
	  #if USE_ALLOCATOR_STATS
		  if (stats_.num_allocations)
			  throw allocate_error(allocate_errc::outstanding_allocations);
	  #endif

	  if (!valid())
		  throw allocate_error(allocate_errc::corrupt);

	  sbb_destroy((sbb_t)heap_);
	  heap_ = nullptr;
	  heap_size_ = 0;
  }
	return memory;
}

sbb_allocator::sbb_allocator(sbb_allocator&& that)
{
	operator=(std::move(that));
}

sbb_allocator& sbb_allocator::operator=(sbb_allocator&& that)
{
	if (this != &that)
	{
		heap_ = that.heap_; that.heap_ = nullptr;
		heap_size_ = that.heap_size_; that.heap_size_ = 0;
		stats_ = that.stats_; that.stats_ = allocator_stats();
	}

	return *this;
}

allocator_stats sbb_allocator::get_stats() const
{
	allocator_stats s = stats_;
	walk_stats ws;
	sbb_walk_heap((sbb_t)heap_, find_largest_free_block, &ws);
	s.largest_free_block_bytes = ws.largest_free_block_bytes;
	s.num_free_blocks = ws.num_free_blocks;
	return s;
}

const void* sbb_allocator::bookkeeping() const
{
	return sbb_bookkeeping((sbb_t)heap_);
}

const void* sbb_allocator::base() const
{
	return sbb_arena((sbb_t)heap_);
}

size_t sbb_allocator::capacity() const
{
	return sbb_arena_bytes((sbb_t)heap_);
}

void* sbb_allocator::allocate(size_t bytes, const char* label, const allocate_options& options)
{
	bytes = std::max(bytes, size_t(1));
	size_t align = options.convert_alignment();
	
	void* p;

	if (align == 16)
	{
		p = sbb_malloc((sbb_t)heap_, bytes);
		SBB_TRACE("sbb_malloc", p, nullptr, bytes, align, label);
	}

	else
	{
		p = sbb_memalign((sbb_t)heap_, align, bytes);
		SBB_TRACE("sbb_memalign", p, nullptr, bytes, align, label);
	}

	if (p)
	{
		size_t block_size = nextpow2(bytes);//sbb_block_size(p);

		#if USE_ALLOCATOR_STATS
			stats_.num_allocations++;
			stats_.num_allocations_peak = std::max(stats_.num_allocations_peak, stats_.num_allocations);
			stats_.allocated_bytes += block_size;
			stats_.allocated_bytes_peak = std::max(stats_.allocated_bytes_peak, stats_.allocated_bytes);
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

void* sbb_allocator::reallocate(void* ptr, size_t bytes)
{
	size_t block_size = ptr ? sbb_block_size((sbb_t)heap_, ptr) : 0;
	void* p = sbb_realloc((sbb_t)heap_, ptr, bytes);
	SBB_TRACE("sbb_realloc", p, ptr, bytes, 0, "");
	if (p)
	{
		size_t diff = nextpow2(bytes)/*sbb_block_size(p)*/ - block_size;

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

void sbb_allocator::deallocate(void* ptr)
{
	if (ptr)
	{
		const size_t block_size = sbb_block_size((sbb_t)heap_, ptr); // kinda slow... should sbb be modified to return block size freed?
		sbb_free((sbb_t)heap_, ptr);
		SBB_TRACE("sbb_free", ptr, nullptr, 0, 0, "");

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

size_t sbb_allocator::size(void* ptr) const 
{
	return sbb_block_size((sbb_t)heap_, ptr);
}

size_t sbb_allocator::offset(void* ptr) const
{
	return owns(ptr) ? size_t((uint8_t*)ptr - (uint8_t*)sbb_arena((sbb_t)heap_)) : size_t(-1);
}

void* sbb_allocator::ptr(size_t offset) const
{
	void* p = (uint8_t*)sbb_arena((sbb_t)heap_) + offset;
	return owns(p) ? p : nullptr;
}

bool sbb_allocator::owns(void* ptr) const
{
	return ptr >= heap_ && ptr < ((uint8_t*)sbb_arena((sbb_t)heap_) + heap_size_);
}

bool sbb_allocator::valid() const
{
	return heap_ && sbb_check_heap((sbb_t)heap_);
}

void sbb_allocator::reset()
{
	if (!heap_ || !heap_size_)
		throw allocate_error(allocate_errc::invalid);
	heap_ = sbb_create(sbb_arena((sbb_t)heap_), heap_size_, min_block_size_, heap_);

	#if USE_ALLOCATOR_STATS
		stats_ = allocator_stats();
		stats_.capacity_bytes = heap_size_ - sbb_overhead((sbb_t)heap_);
	#endif
}

void sbb_allocator::walk_heap(allocate_heap_walk_fn walker, void* user)
{
	if (walker)
		sbb_walk_heap((sbb_t)heap_, walker, user);
}

}
