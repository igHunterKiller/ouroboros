// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oConcurrency/concurrent_hash_map.h>
#include <oCore/bit.h>

namespace ouro {

concurrent_hash_map::size_type concurrent_hash_map::calc_size(size_type capacity)
{
	const size_type n = __max(8, nextpow2(capacity * 2));
	const size_type key_bytes = n * sizeof(std::atomic<key_type>);
	const size_type value_bytes = n * sizeof(std::atomic<value_type>);
	return key_bytes + value_bytes;
}

concurrent_hash_map::concurrent_hash_map()
	: keys_(nullptr)
	, values_(nullptr)
	, modulo_mask_(0)
{}

concurrent_hash_map::concurrent_hash_map(concurrent_hash_map&& that)
	: keys_(that.keys_)
	, values_(that.values_)
	, modulo_mask_(that.modulo_mask_)
{
	that.deinitialize();
}

concurrent_hash_map::concurrent_hash_map(void* memory, size_type capacity)
{
	initialize(memory, capacity);
}

concurrent_hash_map::~concurrent_hash_map()
{
	deinitialize();
}

concurrent_hash_map& concurrent_hash_map::operator=(concurrent_hash_map&& that)
{
	if (this != &that)
	{
		deinitialize();
		modulo_mask_ = that.modulo_mask_; that.modulo_mask_ = 0;
		keys_ = that.keys_; that.keys_ = nullptr;
		values_ = that.values_; that.values_ = nullptr;
	}
	return *this;
}

void concurrent_hash_map::initialize(void* memory, size_type capacity)
{
	allocate_options req_opts(required_alignment);
	if (!req_opts.aligned(memory))
		throw allocate_error(allocate_errc::alignment);
	const size_type n = std::max(8u, nextpow2(capacity * 2));
	const size_type key_bytes = n * sizeof(std::atomic<key_type>);
	const size_type value_bytes = n * sizeof(std::atomic<value_type>);
	const size_type req = key_bytes + value_bytes;
	modulo_mask_ = n - 1;
	keys_ = (std::atomic<key_type>*)memory;
	values_ = (std::atomic<value_type>*)((uint8_t*)memory + key_bytes);
	memset(keys_, 0xff/*nullkey*/, key_bytes);
	memset(values_, nullidx, value_bytes);
}

void* concurrent_hash_map::deinitialize()
{
	void* p = keys_;
	if (p)
	{
		modulo_mask_ = 0;
		keys_ = nullptr;
		values_ = nullptr;
	}
	return p;
}

void concurrent_hash_map::clear()
{
	const size_type n = modulo_mask_ + 1;
	const size_type key_bytes = n * sizeof(std::atomic<key_type>);
	const size_type value_bytes = n * sizeof(std::atomic<value_type>);
	memset(keys_, 0xff/*nullkey*/, key_bytes);
	memset(values_, nullidx, value_bytes);
}

concurrent_hash_map::size_type concurrent_hash_map::size() const
{
	size_type n = 0;
	for (uint32_t i = 0; i <= modulo_mask_; i++)
		if (keys_[i].load(std::memory_order_relaxed) != nullkey && 
			values_[i].load(std::memory_order_relaxed) != nullidx)
			n++;
	return n;
}

concurrent_hash_map::size_type concurrent_hash_map::reclaim()
{
	size_type n = 0;
	uint32_t i = 0;
	while (i <= modulo_mask_)
	{
		if (values_[i] == nullidx && keys_[i] != nullkey)
		{
			keys_[i] = nullkey;
			n++;

			uint32_t ii = (i + 1) & modulo_mask_;
			while (keys_[ii] != nullkey)
			{
				if (values_[ii] == nullidx)
				{
					keys_[ii] = nullkey;
					n++;
				}

				else if ((keys_[ii] & modulo_mask_) != ii) // move if key is misplaced due to collision
				{
					key_type k = keys_[ii];
					value_type v = values_[ii];
					keys_[ii] = nullkey;
					values_[ii] = nullidx;
					set(k, v);
				}

				i = __max(i, ii);
				ii = (ii + 1) & modulo_mask_;
			}
		}
		else
			i++;
	}

	return n;
}

concurrent_hash_map::size_type concurrent_hash_map::migrate(concurrent_hash_map& that, size_type max_moves)
{
	size_type n = 0;
	uint32_t i = 0;
	while (i <= modulo_mask_)
	{
		if (values_[i] != nullidx && keys_[i] != nullkey)
		{
			that.set(keys_[i], values_[i]);
			remove(keys_[i]);
			if (++n >= max_moves)
				break;
		}
		
		i++;
	}

	return n;
}

concurrent_hash_map::value_type concurrent_hash_map::set(const key_type& key, const value_type& value)
{
	if (key == nullkey)
		throw std::invalid_argument("key must be non-zero");
	for (key_type k = key, j = 0;; k++, j++)
	{
		if (j > modulo_mask_)
			throw std::length_error("concurrent_hash_map full");

		k &= modulo_mask_;
		std::atomic<key_type>& stored = keys_[k];
		key_type probed = stored.load(std::memory_order_relaxed);
		if (probed != key)
		{
			if (probed != nullkey)
				continue;
			key_type prev = nullkey;
			stored.compare_exchange_strong(prev, key, std::memory_order_relaxed);
			if (prev != nullkey && prev != key)
				continue;
		}
		return values_[k].exchange(value, std::memory_order_relaxed);
	}
}
	
concurrent_hash_map::value_type concurrent_hash_map::get(const key_type& key) const
{
	if (key == nullkey)
		throw std::invalid_argument("key must be non-zero");
	for (key_type k = key;; k++)
	{
		k &= modulo_mask_;
		std::atomic<key_type>& stored = keys_[k];
		key_type probed = stored.load(std::memory_order_relaxed);
		if (probed == key)
			return values_[k].load(std::memory_order_relaxed);
		if (probed == nullkey)
			return nullidx;
	}
}

}
