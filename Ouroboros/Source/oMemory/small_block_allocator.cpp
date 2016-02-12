// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oMemory/small_block_allocator.h>
#include <oMemory/allocate.h>
#include <oCore/byte.h>

namespace ouro {

small_block_allocator::small_block_allocator()
{
	blksizes_.fill(chunk_t::nullidx);
	partials_.fill(chunk_t::nullidx);
}

small_block_allocator::small_block_allocator(small_block_allocator&& that)
	: chunks_(std::move(that.chunks_))
	, blksizes_(std::move(that.blksizes_))
	, partials_(std::move(that.partials_))
{
}

small_block_allocator::small_block_allocator(void* memory, size_type bytes, const uint16_t* block_sizes, size_type num_block_sizes)
{
	initialize(memory, bytes, block_sizes, num_block_sizes);
}

small_block_allocator::~small_block_allocator()
{
	deinitialize();
}

small_block_allocator& small_block_allocator::operator=(small_block_allocator&& that)
{
	if (this != &that)
	{
		deinitialize();

		chunks_ = std::move(that.chunks_);
		blksizes_ = std::move(that.blksizes_);
		partials_ = std::move(that.partials_);
	}

	return *this;
}

void small_block_allocator::initialize(void* memory, size_type bytes, const uint16_t* block_sizes, size_type num_block_sizes)
{
	if (!aligned(memory, chunk_size))
		throw allocate_error(allocate_errc::alignment);
	
  if (!aligned(bytes, chunk_size))
		throw allocate_error(allocate_errc::size_alignment);

	memcpy(blksizes_.data(), block_sizes, sizeof(uint16_t) * std::min(max_num_block_sizes, num_block_sizes));
	
	// fill in any extra room with repeated value
	for (auto i = num_block_sizes, j = num_block_sizes-1; i < max_num_block_sizes; i++)
		blksizes_[i] = blksizes_[j];

	partials_.fill(chunk_t::nullidx);

	chunks_.initialize(memory, bytes, chunk_size);
}

void* small_block_allocator::deinitialize()
{
	blksizes_.fill(chunk_t::nullidx);
	partials_.fill(chunk_t::nullidx);
	return chunks_.deinitialize();
}

void small_block_allocator::remove_chunk(chunk_t* c)
{
	if (c->prev != chunk_t::nullidx)
	{
		chunk_t* prev = (chunk_t*)chunks_.pointer(c->prev);
		prev->next = c->next;
		c->prev = chunk_t::nullidx;
	}

	else
	{
		// is head, so update that pointer
		partials_[c->bin] = c->next;
	}

	if (c->next != chunk_t::nullidx)
	{
		chunk_t* next = (chunk_t*)chunks_.pointer(c->next);
		next->prev = c->prev;
		c->next = chunk_t::nullidx;
	}
}

uint16_t small_block_allocator::find_bin(size_t size)
{
	uint16_t lower = 0, upper = max_num_block_sizes-1;
	do
	{
		const uint16_t mid = (upper+lower) / 2;
		const size_t blksize = blksizes_[mid];
		if (size == blksize)
			return mid;
		else if (size < blksize)
			upper = mid - 1;
		else
			lower = mid + 1;

	} while (upper >= lower);

	return chunk_t::nullidx;
}

void* small_block_allocator::allocate(size_t size)
{
	uint16_t bin = find_bin(size);
	if (bin == chunk_t::nullidx)
		return nullptr;

	uint16_t chunki = partials_[bin];
	chunk_t* c = chunki == chunk_t::nullidx ? nullptr : (chunk_t*)chunks_.pointer(chunki);
			
	// if there's no partial chunks_ allocate a new one to service this size
	if (!c)
	{
		chunki = (uint16_t)chunks_.allocate_index();
		if (chunki == chunk_t::nullidx)
			return nullptr;
		c = (chunk_t*)chunks_.pointer(chunki);
		void* mem = align(&c[1], sizeof(void*));
		const size_t bytes = (chunks_.block_size() - size_t((uint8_t*)mem - (uint8_t*)c));
		c->pool.initialize(mem, size_type(bytes), size_type(size));
		c->prev = chunk_t::nullidx;
		c->next = chunk_t::nullidx;
		c->bin = bin;
		partials_[bin] = chunki;
	}

	void* p = c->pool.allocate();
			
	// if allocating the last free block, remove the chunk from the list since it 
	// cannot service future requests.
	if (c->pool.empty())
		remove_chunk(c);
	return p;
}

void small_block_allocator::deallocate(void* ptr)
{
	if (!ptr)
		return;

	// aligning the ptr will get to its owning chunk
	chunk_t* c = align_down((chunk_t*)ptr, chunk_size);

	const bool was_empty = c->pool.empty(); // record state prior to insertion
	c->pool.deallocate(ptr);

	// if this is the last block then we can free up this chunk for other sizes
	if (c->pool.full())
	{
		remove_chunk(c);
		c->pool.deinitialize(); // not really needed, maybe make this debug-only
		chunks_.deallocate(c);
	}

	else if (was_empty) // empty chunks_ are in limbo, so reattach this to partials_
	{
		const uint16_t bin = c->bin;
		const uint16_t head = partials_[bin];
		const uint16_t idx = (uint16_t)chunks_.index(c);
		c->prev = chunk_t::nullidx;
		c->next = head;
		partials_[bin] = idx;

		if (head != chunk_t::nullidx)
		{
			chunk_t* old_head = (chunk_t*)chunks_.pointer(head);
			old_head->prev = idx;
		}
	}
}

}
