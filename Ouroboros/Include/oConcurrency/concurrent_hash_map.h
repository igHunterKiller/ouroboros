// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A simple concurrent hash map
// http://preshing.com/20130605/the-worlds-simplest-lock-free-hash-table/

// Keys cannot be 0. The user must initialize the class with a sentinel/invalid
// value used to mark entries invalid and later sweep derelict keys. There is no 
// concurrent remove, so this mark-and-sweep is the alternative.

#pragma once
#include <oCore/bit.h>
#include <oCore/byte.h>
#include <oMemory/allocate.h> // low usage, consider removing this dependency
#include <oMemory/memory.h>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <memory>

namespace ouro {

template<typename keyT, typename valT>
class concurrent_hash_map
{
public:
	typedef uint32_t size_type;
	typedef keyT key_type;
	typedef valT val_type;

	// return true to keep traversing, false to exit early
	typedef bool (*visitor_fn)(key_type key, val_type value, void* user);

	static const memory_alignment required_alignment = memory_alignment::cacheline;
	

	// === non-concurrent api ===

	static size_type calc_size(size_type capacity);                                    // required bytes to hold a number of key-value pairs

	concurrent_hash_map();                                                             // constructs an empty hash map
	concurrent_hash_map(concurrent_hash_map&& that);                                   // moves another hash map into a new one
	concurrent_hash_map(const val_type& invalid, void* memory, size_type bytes);       // use calc_size() to determine bytes
	~concurrent_hash_map();                                                            // dtor
	concurrent_hash_map& operator=(concurrent_hash_map&& that);                        // assignment via move
	
	void      initialize(const val_type& invalid, void* memory, size_type bytes);      // use calc_size() to determine bytes
	void*     deinitialize();                                                          // returns the memory passed to initialize()
	bool      valid()        const { return !!keys_; }                                 // true if this has been initialized
	void      clear();                                                                 // returns the hash map to the empty state
	size_type size()         const;                                                    // number of valid entries (counts O(n))
	bool      empty()        const { return size() == 0; }                             // true if no entries are valid
	size_type occupancy()    const { return (size() * 100) / capacity(); }             // [0,100] percentage of capacity that is valid
	bool      needs_resize() const { return occupancy() > 75; }                        // returns true if performance is degraded due to high occupancy
	size_type reclaim_keys();                                                          // derelict keys can saturate occupancy, call this periodically to eviscerate keys with invalid values
	size_type migrate(concurrent_hash_map& that, size_type max_moves = size_type(-1)); // rehash a limited number of keys from this to that
	void      visit(visitor_fn visitor, void* user);                                   // visit valid values

	template<typename visitorT>
	void visit(visitorT visitor) // std::function form (must be bool visit(key_type k, val_type v)) (return true to keep traversing, false to exit early)
	{
		for (uint32_t i = 0; i <= mod_; i++)
			if (!val_nul(i))
			{
				auto k = keys_[i].load(atm_order);
				auto v = vals_[i].load(atm_order);
				if (!visitor(k, v))
					return;
			}
	}


	// === concurrent api ===

	size_type capacity() const { return mod_ + 1; }                                                             // returns the max storable key-value pairs
	val_type  nul() const { return nul_val_; }                                                                  // returns the nul val set at init time
	bool      add(const key_type& key, const val_type& value);                                                  // sets key to value if the prior key or value is nul (un-reclaimed keys do not fail)
	bool      cas(const key_type& key, val_type& old_value, const key_type& new_value, val_type*& val_pointer); // compare-and-swap rules: if key not found behave as add (old_value <- nul); if key then only assign val if it's still old_value
	val_type  set(const key_type& key, const val_type& value, val_type*& val_pointer);                          // sets key to value and returns prior; out_index receives the index into vals_ to which key evaluated
	val_type  nix(const key_type& key) { return set(key, nul_val_); }                                           // flags key as valid, but derelict so reclaim_keys() can later reset the key
	val_type  get(const key_type& key) const;                                                                   // returns nul if no key/value exists

	bool      cas(const key_type& key, val_type& old_value, const key_type& new_value) { val_type* dummy; return cae(key, old_value, new_value, dummy); }
	val_type  set(const key_type& key, const val_type& value) { val_type* dummy; return set(key, value, dummy); }
	
private:
	concurrent_hash_map(const concurrent_hash_map&);
	const concurrent_hash_map& operator=(const concurrent_hash_map&);

	static const key_type nul_key_ = key_type(0);

	typedef std::atomic<key_type>  atm_key_t;
	typedef std::atomic<val_type>  atm_val_t;
	static const std::memory_order atm_order = std::memory_order_seq_cst; // std::memory_order_relaxed

	bool key_nul(uint32_t index) const { return keys_[index] == nul_key_; }
	bool val_nul(uint32_t index) const { return vals_[index] == nul_val_; }

	atm_key_t* keys_;    // all keys in the hash map
	atm_val_t* vals_;    // all values in the hash map
	val_type   nul_val_; // invalid value used to flag a key as derelict
	uint32_t   mod_;     // modulo treated as a pow-of-two size - 1 'and' mask
};

template<typename keyT, typename valT>
typename concurrent_hash_map<keyT, valT>::size_type concurrent_hash_map<keyT, valT>::calc_size(size_type capacity)
{
	const size_type max_align = (size_type)std::max(sizeof(atm_key_t), sizeof(atm_val_t));
	const size_type n         = (size_type)std::max(size_type(8), (size_type)nextpow2(capacity * 2)); // 2x memory and a power-of-two size is required for performance
	const size_type key_bytes = align(n * (size_type)sizeof(atm_key_t), max_align);                   // ensure vals_ can be naturally aligned
	const size_type val_bytes =       n * (size_type)sizeof(atm_val_t);
	return key_bytes + val_bytes;
}

template<typename keyT, typename valT>
concurrent_hash_map<keyT, valT>::concurrent_hash_map()
	: keys_(nullptr)
	, vals_(nullptr)
	, mod_(0)
{}

template<typename keyT, typename valT>
concurrent_hash_map<keyT, valT>::concurrent_hash_map(concurrent_hash_map&& that)
	: keys_(that.keys_)
	, vals_(that.vals_)
	, mod_(that.mod_)
{
	that.deinitialize();
}

template<typename keyT, typename valT>
concurrent_hash_map<keyT, valT>::concurrent_hash_map(const val_type& invalid, void* memory, size_type capacity)
{
	initialize(invalid, memory, capacity);
}

template<typename keyT, typename valT>
concurrent_hash_map<keyT, valT>::~concurrent_hash_map()
{
	deinitialize();
}

template<typename keyT, typename valT>
concurrent_hash_map<keyT, valT>& concurrent_hash_map<keyT, valT>::operator=(concurrent_hash_map<keyT, valT>&& that)
{
	if (this != &that)
	{
		deinitialize();
		keys_    = that.keys_;    that.keys_    = nullptr;
		vals_    = that.vals_;    that.vals_    = nullptr;
		nul_val_ = that.nul_val_; that.nul_val_ = 0;
		mod_     = that.mod_;     that.mod_     = 0;
	}
	return *this;
}

template<typename keyT, typename valT>
void concurrent_hash_map<keyT, valT>::initialize(const val_type& invalid, void* memory, size_type capacity)
{
	allocate_options req_opts(required_alignment);
	if (!req_opts.aligned(memory))
		throw allocate_error(allocate_errc::alignment);

	// copy-paste of calc_size is unfortunate, some innards are required here
	const size_type max_align = (size_type)std::max(sizeof(atm_key_t), sizeof(atm_val_t));
	const size_type n         = (size_type)std::max(size_type(8), (size_type)nextpow2(capacity * 2));
	const size_type key_bytes = align(n * (size_type)sizeof(atm_key_t), max_align);

	keys_    = (atm_key_t*)memory;
	vals_    = (atm_val_t*)((uint8_t*)memory + key_bytes);
	nul_val_ = invalid;
	mod_     = uint32_t(n - 1);
	clear();
}

template<typename keyT, typename valT>
void* concurrent_hash_map<keyT, valT>::deinitialize()
{
	void* p = keys_;
	if (p)
	{
		keys_    = nullptr;
		vals_    = nullptr;
		nul_val_ = 0;
		mod_     = 0;
	}
	return p;
}

template<typename keyT, typename valT>
void concurrent_hash_map<keyT, valT>::clear()
{
	const size_type max_align = (size_type)std::max(sizeof(atm_key_t), sizeof(atm_val_t));
	const size_type n         = capacity();
	const size_type key_bytes = align(n * (size_type)sizeof(atm_key_t), max_align);
	const size_type val_bytes =       n * (size_type)sizeof(atm_val_t);

	memset(           keys_, nul_key_, key_bytes); // zero key implied an available slot, so no user key can be zero
	memset((val_type*)vals_, nul_val_, val_bytes);
}

template<typename keyT, typename valT>
typename concurrent_hash_map<keyT, valT>::size_type concurrent_hash_map<keyT, valT>::size() const
{
	size_type n = 0;
	for (uint32_t i = 0; i <= mod_; i++)
		if (!key_nul(i) && !val_nul(i))
			n++;
	return n;
}

template<typename keyT, typename valT>
typename concurrent_hash_map<keyT, valT>::size_type concurrent_hash_map<keyT, valT>::reclaim_keys()
{
	size_type n = 0;
	uint32_t i = 0;
	while (i <= mod_)
	{
		if (val_nul(i) && !key_nul(i))
		{
			keys_[i] = nul_key_;
			n++;

			uint32_t ii = (i + 1) & mod_;
			while (!key_nul(ii))
			{
				if (val_nul(ii))
				{
					keys_[ii] = nul_key_;
					n++;
				}

				else if ((keys_[ii] & mod_) != ii) // move if key is misplaced due to collision
				{
					key_type k = keys_[ii];
					val_type v = vals_[ii];
					keys_[ii]  = nul_key_;
					vals_[ii]  = nul_val_;
					set(k, v);
				}

				i = std::max(i, ii);
				ii = (ii + 1) & mod_;
			}
		}
		else
			i++;
	}

	return n;
}

template<typename keyT, typename valT>
typename concurrent_hash_map<keyT, valT>::size_type concurrent_hash_map<keyT, valT>::migrate(concurrent_hash_map& that, size_type max_moves)
{
	size_type n = 0;
	uint32_t i = 0;
	while (i <= mod_)
	{
		if (!val_nul(i) && !key_nul(i))
		{
			that.set(keys_[i], vals_[i]);
			remove(keys_[i]);
			if (++n >= max_moves)
				break;
		}
		
		i++;
	}

	return n;
}

template<typename keyT, typename valT>
void concurrent_hash_map<keyT, valT>::visit(visitor_fn visitor, void* user)
{
	for (uint32_t i = 0; i <= mod_; i++)
		if (!val_nul(i))
		{
			auto k = keys_[i].load(atm_order);
			auto v = keys_[i].load(atm_order);
			if (!visitor(k, v))
				return;
		}
}

template<typename keyT, typename valT>
bool concurrent_hash_map<keyT, valT>::add(const key_type& key, const val_type& value)
{
	if (key == nul_key_)
		throw std::invalid_argument("key must be non-zero");

	for (key_type k = key, j = 0;; k++, j++) // j ensures one loop, k can wrap over the memory
	{
		// should this return false because the add failed? or is it uniformly 
		// an invalid operation to try to add/set on a full hash?
		if (j > mod_)
			throw std::length_error("concurrent_hash_map full");

		k &= mod_;
		atm_key_t& atm_key = keys_[k];
		key_type probed = atm_key.load(atm_order);

		if (probed == key)
			return false;

		if (probed == nul_key_ && atm_key.compare_exchange_strong(probed, key, atm_order))
		{
			vals_[k].exchange(value, atm_order);
			return true;
		}
	}
}

template<typename keyT, typename valT>
bool concurrent_hash_map<keyT, valT>::cas(const key_type& key, val_type& old_value, const key_type& new_value, val_type*& val_pointer)
{
	if (key == nul_key_)
		throw std::invalid_argument("key must be non-zero");

	for (key_type k = key, j = 0;; k++, j++) // j ensures one loop, k can wrap over the memory
	{
		// key was not found and a new one could not be added
		if (j > mod_)
			throw std::length_error("concurrent_hash_map full");

		k &= mod_;
		atm_key_t& atm_key = keys_[k];
		key_type probed = atm_key.load(atm_order);
		if (probed != key)
		{
			// invoke the 'linear' in linear probe hashmap: skip a seemingly valid key and look for the next available slot
			if (probed != nul_key_)
				continue;

			// try inserting a new key and double-test to protect against another thread inserting the same key (so ignore cas's return value)
			key_type prev = nul_key_;
			atm_key.compare_exchange_strong(prev, key, atm_order);
			if (prev != nul_key_ && prev != key)
				continue;
		}

		// @tony: this feels a bit heavyweight since the key needs to be reevalated by calling code - perhaps
		// pass a callback that cas-loops here? see what usage looks like and swing back to optimize this if necessary

		// either the key is found or a new one was successfully written: ready to update the value, but only if it still matches the old value

		val_pointer = (val_type*)vals_ + k;

		return vals_[k].compare_exchange_strong(old_value, new_value, atm_order);
	}
}

template<typename keyT, typename valT>
typename concurrent_hash_map<keyT, valT>::val_type concurrent_hash_map<keyT, valT>::set(const key_type& key, const val_type& value, val_type*& val_pointer)
{
	if (key == nul_key_)
		throw std::invalid_argument("key must be non-zero");

	for (key_type k = key, j = 0;; k++, j++) // j ensures one loop, k can wrap over the memory
	{
		// key was not found and a new one could not be added
		if (j > mod_)
			throw std::length_error("concurrent_hash_map full");

		k &= mod_;
		atm_key_t& atm_key = keys_[k];
		key_type probed = atm_key.load(atm_order);
		if (probed != key)
		{
			// invoke the 'linear' in linear probe hashmap: skip a seemingly valid key and look for the next available slot
			if (probed != nul_key_)
				continue;

			// try inserting a new key and double-test to protect against another thread inserting the same key (so ignore cas's return value)
			key_type prev = nul_key_;
			atm_key.compare_exchange_strong(prev, key, atm_order);
			if (prev != nul_key_ && prev != key)
				continue;
		}

		val_pointer = (val_type*)vals_ + k;

		// either the key is found or a new one was successfully written: ready to update the value
		return vals_[k].exchange(value, atm_order);
	}
}

template<typename keyT, typename valT>
typename concurrent_hash_map<keyT, valT>::val_type concurrent_hash_map<keyT, valT>::get(const key_type& key) const
{
	if (key == nul_key_)
		throw std::invalid_argument("key must be non-zero");

	// linear probe hashes are similar to strings: the start is at the key and 
	// if iteration reaches a nul key then there's no chance of a valid value
	for (key_type k = key;; k++) 
	{
		k &= mod_;
		atm_key_t& atm_key = keys_[k];
		key_type probed = atm_key.load(atm_order);
		if (probed == key)
			return vals_[k].load(atm_order);
		if (probed == nul_key_)
			return nul_val_;
	}
}

}
