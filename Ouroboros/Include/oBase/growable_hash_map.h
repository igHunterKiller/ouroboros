// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A wrapper for hash_map and an allocator that allows the hash to grow 
// when it occupancy causes performance degradation.

#pragma once
#include <oCore/hash_map.h>
#include <oMemory/allocate.h>
#include <functional>
#include <cstdint>
#include <memory.h>
#include <stdlib.h>

namespace ouro {

template<typename keyT, typename valT>
class growable_hash_map : public hash_map<keyT, valT>
{
	growable_hash_map                 (const growable_hash_map&);/* = delete; */
	const growable_hash_map& operator=(const growable_hash_map&);/* = delete; */

public:
	typedef typename hash_map<keyT, valT> base_type; 
	typedef typename base_type::size_type size_type;
	typedef typename base_type::key_type  key_type;
	typedef typename base_type::val_type  val_type;

	growable_hash_map () {}
	~growable_hash_map() { void* p = base_type::deinitialize(); alloc_.deallocate(p); }
	
	growable_hash_map(size_type capacity, const char* alloc_label = "growable_hash_map ctor", const allocator& a = default_allocator) { initialize(capacity, alloc_label, a); }
	growable_hash_map(growable_hash_map&& that) : hash_map(std::move((base_type&&)that)), alloc_(std::move(that.alloc_)) {}
		
	// moves anothr hash map into this, throws if this is not yet deinitialized
	growable_hash_map& operator=(growable_hash_map&& that)
	{
		if (this != &that)
		{
			*(base_type*)this = std::move(that);
			alloc_ = std::move(that.hash_);
		}
		return *this;
	}

	// this class evaluates to true if properly initialized or false if not
	operator bool() const { return !!*(base_type*)this; }

	void initialize(size_type capacity, const char* alloc_label = "growable_hash_map initialize", const allocator& a = default_allocator)
	{
		base_type::initialize(a.allocate(calc_size(capacity), alloc_label), capacity);
		alloc_ = a;
	}

	// deinitializes the hash map and returns the memory that passed to initialize()
	void deinitialize()
	{
		alloc_.deallocate(base_type::deinitialize());
	}

	// allocates and rehashes entries into a newly sized version of this hash map
	void resize(const size_type& new_capacity, const char* alloc_label = "growable_hash_map resize")
	{
		growable_hash_map h(new_capacity, alloc_label, alloc_);
		enumerate([&](const key_type& key, val_type& val)->bool
		{
			h.add(key, std::move(val));
			return true;
		});
		swap(h);
		alloc_.deallocate(((growable_hash_map::base_type&)h).deinitialize());
	}

	// adds a key/val pair and returns true. Noops and returns false if key 
	// already exists
	bool add(const key_type& key, const val_type& val)
	{
		if (needs_resize())
			resize(size() * 2);
		return base_type::add(key, val);
	}

	// moves a key/val pair into the hash map and returns true. Noops and 
	// returns false if key already exists
	bool add(const key_type& key, val_type&& val)
	{
		if (needs_resize())
			resize(size() * 2);
		return base_type::add(key, std::move(val));
	}

	// adds a new or overwrites an existing key/val pair
	void add_or_set(const key_type& key, const val_type& val)
	{
		if (needs_resize())
			resize(size() * 2);
		base_type::add_or_set(key, val);
	}

	// moves a key/value pair, overwriting any existing one
	void add_or_set(const key_type& key, val_type&& val)
	{
		if (needs_resize())
			resize(size() * 2);
		base_type::add_or_set(key, std::move(val));
	}

private:
	allocator alloc_;
};

}
