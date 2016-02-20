// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMemory/concurrent_pool.h>
#include <oMemory/allocate.h>
#include <algorithm>
#include <stdexcept>

namespace ouro {

static_assert(sizeof(concurrent_pool) == oCACHE_LINE_SIZE, "unexpected class size");
static_assert(sizeof(std::atomic_uint) == sizeof(unsigned int), "mismatch atomic size");

struct tagged
{
	tagged() {}
	tagged(unsigned int _all) : all(_all) {}

	union
	{
		unsigned int all;
		struct
		{
			unsigned int index : 24;
			unsigned int tag : 8;
		};
	};
};

concurrent_pool::size_type concurrent_pool::calc_size(size_type block_size, size_type capacity)
{
	return std::max(block_size, size_type(sizeof(index_type))) * capacity;
}

concurrent_pool::size_type concurrent_pool::calc_capacity(size_type bytes, size_type block_size)
{
	return bytes / std::max(block_size, size_type(sizeof(index_type)));
}

concurrent_pool::concurrent_pool()
	: next_(nullptr)
	, blocks_(nullptr)
	, stride_(0)
	, nblocks_(0)
{
	tagged h(nullidx);
	head_ = h.all;
}

concurrent_pool::concurrent_pool(concurrent_pool&& that)
	: next_(that.next_)
	, blocks_(that.blocks_)
	, stride_(that.stride_)
	, nblocks_(that.nblocks_)
{ 
	head_.store(that.head_);
	that.deinitialize();
}

concurrent_pool::concurrent_pool(void* arena, size_type bytes, size_type block_size)
	: blocks_(nullptr)
	, stride_(0)
	, nblocks_(0)
{
	initialize(arena, bytes, block_size);
}

concurrent_pool::~concurrent_pool()
{
	deinitialize();
}

concurrent_pool& concurrent_pool::operator=(concurrent_pool&& that)
{
	if (this != &that)
	{
		deinitialize();

		next_ = that.next_; that.next_ = nullptr;
		blocks_ = that.blocks_; that.blocks_ = nullptr;
		stride_ = that.stride_; that.stride_ = 0;
		nblocks_ = that.nblocks_; that.nblocks_ = 0;
		head_.store(that.head_); that.head_ = nullidx;
	}

	return *this;
}

void concurrent_pool::initialize(void* arena, size_type bytes, size_type block_size)
{
	if (!arena)
		throw allocate_error(allocate_errc::invalid_ptr);

	if (block_size < sizeof(index_type))
		throw allocate_error(allocate_errc::size_alignment);

	const size_type capacity = calc_capacity(bytes, block_size);
	if (!capacity || capacity > max_capacity())
		throw allocate_error(allocate_errc::invalid_capacity);

	head_ = 0;
	blocks_ = (uint8_t*)arena;
	stride_ = block_size;
	nblocks_ = capacity;
	const index_type n = nblocks_ - 1;
	for (index_type i = 0; i < n; i++)
		*(index_type*)(stride_*i + blocks_) = i + 1;
	*(index_type*)(stride_*n + blocks_) = nullidx;
}

void* concurrent_pool::deinitialize()
{
	void* p = blocks_;
	blocks_ = nullptr;
	stride_ = 0;
	nblocks_ = 0;
	head_ = nullidx;
	return p;
}

concurrent_pool::size_type concurrent_pool::count_free() const
{
	tagged o(head_);
	size_type n = 0;
	index_type i = o.index;
	while (i != nullidx)
	{
		n++;
		i = *(index_type*)(stride_*i + blocks_);
	}
	return n;
}

bool concurrent_pool::empty() const
{
	tagged o(head_);
	return o.index == nullidx;
}

concurrent_pool::index_type concurrent_pool::allocate_index()
{
	index_type i;
	tagged n, o(head_);
	do
	{	i = o.index;
		if (i == nullidx)
			break;
		n.tag = o.tag + 1;
		n.index = *(index_type*)(stride_*i + blocks_);
	} while (!head_.compare_exchange_strong(o.all, n.all));
	return i;
}

void concurrent_pool::deallocate(index_type index)
{
	tagged n, o(head_);
	do
	{	*(index_type*)(stride_*index + blocks_) = o.index;
		n.tag = o.tag + 1;
		n.index = index;
	} while (!head_.compare_exchange_strong(o.all, n.all));
}

// convert between allocated index and pointer values
void* concurrent_pool::pointer(index_type index) const
{
	return index != nullidx ? (stride_*index + blocks_) : nullptr;
}

concurrent_pool::index_type concurrent_pool::index(void* ptr) const
{
	return ptr ? index_type(((uint8_t*)ptr - blocks_) / stride_) : nullidx;
}

}
