// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/pool.h>
#include <oMemory/allocate.h>
#include <stdexcept>

namespace ouro {

pool::size_type pool::calc_size(size_type capacity, size_type block_size)
{
	return std::max(block_size, size_type(sizeof(index_type))) * capacity;
}

pool::size_type pool::calc_capacity(size_type bytes, size_type block_size)
{
	return bytes / std::max(block_size, size_type(sizeof(index_type)));
}

pool::pool()
	: blocks_(nullptr)
	, stride_(0)
	, nblocks_(0)
	, nfree_(0)
	, head_(nullidx)
{}

pool::pool(pool&& that)
	: blocks_(that.blocks_)
	, stride_(that.stride_)
	, nblocks_(that.nblocks_)
	, nfree_(that.nfree_)
	, head_(that.head_)
{ 
	that.deinitialize();
}

pool::pool(void* arena, size_type bytes, size_type block_size)
	: blocks_(nullptr)
	, stride_(0)
	, nblocks_(0)
	, nfree_(0)
	, head_(nullidx)
{
	initialize(arena, bytes, block_size);
}

pool::~pool()
{
	deinitialize();
}

pool& pool::operator=(pool&& that)
{
	if (this != &that)
	{
		deinitialize();

		blocks_ = that.blocks_; that.blocks_ = nullptr;
		stride_ = that.stride_; that.stride_ = 0;
		nblocks_ = that.nblocks_; that.nblocks_ = 0;
		nfree_ = that.nfree_; that.nfree_ = 0;
		head_ = that.head_; that.head_ = nullidx;
	}

	return *this;
}

void pool::initialize(void* arena, size_type bytes, size_type block_size)
{
	if (!arena)
		throw allocate_error(allocate_errc::invalid_bookkeeping);

	if (block_size < sizeof(index_type))
		throw std::invalid_argument("block_size must be a minimum of 4 bytes");

	const size_type capacity = calc_capacity(bytes, block_size);
	if (!capacity || capacity > max_capacity())
		throw allocate_error(allocate_errc::invalid_bookkeeping);

	head_ = 0;
	blocks_ = (uint8_t*)arena;
	stride_ = block_size;
	nblocks_ = capacity;
	nfree_ = capacity;
	const index_type n = nblocks_ - 1;
	for (index_type i = 0; i < n; i++)
		*(index_type*)(stride_*i + blocks_) = i + 1;
	*(index_type*)(stride_*n + blocks_) = nullidx;
}

void* pool::deinitialize()
{
	void* p = blocks_;
	blocks_ = nullptr;
	stride_ = 0;
	nblocks_ = 0;
	head_ = nullidx;
	return p;
}

pool::index_type pool::allocate_index()
{
	index_type i = index_type(head_);
	if (i == nullidx)
		return nullidx;
	head_ = *(index_type*)pointer(i);
	nfree_--;
	return i;
}

void pool::deallocate(index_type index)
{
	if (!owns(index))
		throw std::invalid_argument("pool does not own the specified index or pointer");
	*(index_type*)pointer(index) = index_type(head_);
	head_ = index;
	nfree_++;
}

// convert between allocated index and pointer values
void* pool::pointer(index_type index) const
{
	return index != nullidx ? (stride_*index + blocks_) : nullptr;
}

pool::index_type pool::index(void* ptr) const
{
	return ptr ? index_type(((uint8_t*)ptr - blocks_) / stride_) : nullidx;
}

}
