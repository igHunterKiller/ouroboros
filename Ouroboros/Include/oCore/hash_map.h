// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Simple open addressing linear probe hash map. Since this will need to rehash 
// on significant removal, there's extra API to mark entries for removal and 
// sweep them up later with a slightly more efficient rehash-all pass.
// https://en.wikipedia.org/wiki/Linear_probing

#pragma once
#include <oCore/assert.h>
#include <oCore/bit.h> // nextpow2
#include <functional>  // std::function
#include <cstring>     // memset
#include <cstdint>

namespace ouro {

template<typename keyT, typename valT>
class hash_map
{
	hash_map                 (const hash_map&);/* = delete; */
	const hash_map& operator=(const hash_map&);/* = delete; */

public:
	typedef uint32_t size_type;
	typedef keyT     key_type;
	typedef valT     val_type;

	static const size_type nullkey = size_type(0);
	static const size_type nullidx = size_type(-1);

	// returns the bytes required to efficiently hash the specified number of items
	static size_type calc_size(size_type capacity)
	{
		const size_type n = calc_aligned_capacity(capacity);
		return n * (sizeof(key_type) + sizeof(val_type));
	}

	// constructs an empty hash map, use initialize() to set it up later
	hash_map() : keys_(nullptr), vals_(nullptr), size_(0), wrap_(0) {}

	~hash_map() { oAssertA(!deinitialize(), "deinitializing hash_map with unfreed memory"); }
	
	// use calc_size() to determine memory size
	hash_map(void* memory, size_type capacity) { initialize(memory, capacity); }

	// moves another hash map into a new one
	hash_map(hash_map&& that) : keys_(that.keys_), vals_(that.vals_), size_(that.size_), wrap_(that.wrap_)
	{
		that.keys_  = nullptr; that.vals_ = nullptr;
		that.size_  = 0;       that.wrap_ = 0;
	}
	
	// moves anothr hash map into this, throws if this is not yet deinitialized
	hash_map& operator=(hash_map&& that)
	{
		if (this != &that)
		{
			if (!!keys_)
				throw std::exception("moving into an initialized hash_map will leak memory");
			keys_ = that.keys_; that.keys_ = nullptr; vals_ = that.vals_; that.vals_ = nullptr;
			size_ = that.size_; that.size_ = 0;       wrap_ = that.wrap_; that.wrap_ = 0;
		}
		return *this;
	}

	// this class evaluates to true if properly initialized or false if not
	operator bool() const { return !!keys_; }

	// use calc_size() to determine memory size
	void initialize(void* memory, size_type capacity)
	{
		static_assert(std::is_integral<key_type>::value, "hash type must be an integral");

		const size_type n = calc_aligned_capacity(capacity);

		keys_  = (key_type*)memory; vals_ = (val_type*)(keys_ + n);
		size_  = 0;                 wrap_ = n - 1;

		init_array(keys_, n);       init_array(vals_, n);
	}

	// deinitializes the hash map and returns the memory that passed to initialize()
	void* deinitialize()
	{
		void* p = nullptr;
		if (keys_)
		{
			const size_type n = wrap_ + 1;
			p                 = keys_;
			
			deinit_array(keys_, n); deinit_array(vals_, n);

			keys_  = nullptr; vals_ = nullptr;
			size_  = 0;       wrap_ = 0;
		}
		return p;
	}

	// returns the hash map to the empty state it was after initialize
	void clear()
	{
		const size_type n = wrap_ + 1;
		deinit_array(keys_, n); deinit_array(vals_, n);
		init_array  (keys_, n); init_array  (vals_, n);
		size_ = 0;
	}

	// returns the absolute capacity within the hash map
	size_type capacity() const { return wrap_; }

	// returns the number of entries in the hash map
	size_type size() const { return size_; }

	// returns true if there are no entries
	bool empty() const { return size() == 0; }

	// returns the percentage used of capacity
	size_type occupancy() const { return (size() * 100) / capacity(); }

	// returns true if performance is degraded due to high occupancy
	// which begins to occur at 75% occupancy
	bool needs_resize() const { return occupancy() > 75; }

	// exchanges the contents of this and that hash map
	void swap(hash_map& that)
	{
		std::swap(keys_,  that.keys_ ); std::swap(vals_, that.vals_);
		std::swap(size_,  that.size_ ); std::swap(wrap_, that.wrap_);
	}

	// returns true if the specified key exists in the 
	bool exists(const key_type& key) const { return find_existing(key) != nullidx; }

	// allocates and rehashes entries into a newly sized version of this hash map
	// and returns the old memory as deinitialize() would
	void* resize(void* new_memory, const size_type& new_capacity)
	{
		hash_map h(new_capacity, alloc_, free_, alloc_label);
		enumerate([&](const key_type& key, val_type& val)->bool
		{
			h.add(key, std::move(val));
			return true;
		});
		swap(h);
		return h.deinitialize();
	}

	// adds a key/val pair and returns true. Noops and returns false if key 
	// already exists
	bool add(const key_type& key, const val_type& val)
	{
		size_type i = find_existing_or_new(key);
		if (i == nullidx || keys_[i] != nullkey || size_ >= wrap_) // full or exists
			return false;
		keys_[i] = key;
		vals_[i] = val;
		size_++;
		return true;
	}

	// moves a key/val pair into the hash map and returns true. Noops and 
	// returns false if key already exists
	bool add(const key_type& key, val_type&& val)
	{
		size_type i = find_existing_or_new(key);
		if (i == nullidx || keys_[i] != nullkey || size_ >= wrap_) // full or exists
			return false;
		keys_[i] = key;
		vals_[i] = std::move(val);
		size_++;
		return true;
	}

	// adds a new or overwrites an existing key/val pair
	void add_or_set(const key_type& key, const val_type& val)
	{
		size_type i = find_existing_or_new(key);
		keys_[i] = key;
		vals_[i] = val;
		size_++;
	}

	// moves a key/value pair, overwriting any existing one
	void add_or_set(const key_type& key, val_type&& val)
	{
		size_type i = find_existing_or_new(key);
		keys_[i] = key;
		vals_[i] = std::move(val);
		size_++;
	}

	// removes a key and returns true, or false key not found
	bool remove(const key_type& key)
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			return false;
		keys_[i] = nullkey;
		vals_[i] = val_type();
		fix_collisions(i);
		size_--;
		return true;
	}

	// replaces a hash key with the mark hash key used for sweeping
	bool mark(const key_type& key, const key_type& mark)
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			return false;
		keys_[i] = mark;
		return true;
	}

	// removes all entries with a key equal to mark
	size_type sweep(const key_type& mark)
	{
		size_type removed = 0;
		// do the fix collisions, but also check for any remove marks along the way
		size_type i = 0;
		while (i <= wrap_)
		{
			if (keys_[i] == mark)
			{
				keys_[i] = nullkey;
				removed++;
				size_--;

				// fix collisions
				size_type ii = (i + 1) & wrap_;
				while (keys_[ii])
				{
					if (keys_[ii] == mark)
					{
						keys_[ii] = nullkey;
						vals_[ii] = val_type();
						removed++;
						size_--;
					}

					else
						fix(ii);

					i = size_max(i, ii);
					ii = (ii + 1) & wrap_;
				}
			}
			else
				i++;
		}

		return removed;
	}

	// sets the existing key to a new value. Returns false if key not found
	bool set(const key_type& key, const val_type& val)
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			return false;
		vals_[i] = val;
		return true;
	}

	// moves a value into the existing key. Returns false if key not found
	bool set(const key_type& key, val_type&& val)
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			return false;
		vals_[i] = std::move(val);
		return true;
	}

	// returns the value associated with key
	val_type get(const key_type& key) const
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			oThrow(std::errc::invalid_argument, "key not found");
		return vals_[i];
	}

	// returns a pointer into the hash map for the value associated with key
	val_type* get_ptr(const key_type& key) const
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			oThrow(std::errc::invalid_argument, "key not found");
		return &vals_[i];
	}

	// returns a poitner into the hash map for the value associated with key
	val_type* get_existing_or_new_ptr(const key_type& key, bool* out_is_new)
	{
		size_type i = find_existing_or_new(key);
		const bool is_new = keys_[i] == nullkey;
		if (is_new) keys_[i] = key;
		if (out_is_new) *out_is_new = is_new;
		return &vals_[i];
	}

	// returns true if out_val receives the existing value associated with key
	// or false if key not found
	bool get(const key_type& key, val_type* out_val) const
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			return false;
		*out_val = vals_[i];
		return true;
	}

	// returns true if out_val receives a pointer to the the existing value 
	// associated with key or false if key not found
	bool get_ptr(const key_type& key, val_type** out_val) const
	{
		size_type i = find_existing(key);
		if (i == nullidx)
			return false;
		*out_val = &vals_[i];
		return true;
	}

	// @tony TODO use a function template and remove dependency on std::function

	void enumerate_const(const bool (*enumerator)(const key_type& key, const val_type& val, void* user), void* user) const
	{
		for (size_type i = 0; i <= wrap_; i++)
			if (keys_[i] != nullkey && !enumerator(keys_[i], vals_[i], user))
				return;
	}

	void enumerate(const bool (*enumerator)(const key_type& key, val_type& val, void* user), void* user)
	{
		for (size_type i = 0; i <= wrap_; i++)
			if (keys_[i] != nullkey && !enumerator(keys_[i], vals_[i], user))
				return;
	}

	void enumerate_const(const std::function<bool(const key_type& key, const val_type& val)>& enumerator) const
	{
		for (size_type i = 0; i <= wrap_; i++)
			if (keys_[i] != nullkey && !enumerator(keys_[i], vals_[i]))
				return;
	}

	void enumerate(const std::function<bool(const key_type& key, val_type& val)>& enumerator) const
	{
		for (size_type i = 0; i <= wrap_; i++)
			if (keys_[i] != nullkey && !enumerator(keys_[i], vals_[i]))
				return;
	}

private:
	size_type find_existing_or_new(const key_type& key)
	{
		if (key)
		{
			key_type* end = keys_ + wrap_ + 1;
			key_type* start = keys_ + (key & wrap_);
			for (key_type* h = start; h < end; h++)
				if (!*h || *h == key)
					return size_type(h - keys_);
			for (key_type* h = keys_; h < start; h++)
				if (!*h || *h == key)
					return size_type(h - keys_);
		}

		return nullidx;
	}

	size_type find_existing(const key_type& key) const
	{
		if (key)
		{
			key_type* end = keys_ + wrap_ + 1;
			key_type* h = keys_ + (key & wrap_);
			while (*h)
			{
				if (*h == key)
					return size_type(h - keys_);
				h++;
				if (h >= end)
					h = keys_;
			}
		}

		return nullidx;
	}

	void fix(const size_type& index)
	{
		if ((keys_[index] & wrap_) != index) // move if key isn't where it supposed to be due to collision
		{
			key_type k = keys_[index];
			keys_[index] = nullkey;
			size_type newi = find_existing_or_new(k); // will only find new since we just moved the existing
			keys_[newi] = k;
			vals_[newi] = std::move(vals_[index]);
		}
	} 

	void fix_collisions(const size_type& removed_index)
	{
		// fix up collisions that might've been after this entry
		size_type i = (removed_index + 1) & wrap_;
		while (keys_[i] != nullkey)
		{
			fix(i);
			i = (i + 1) & wrap_;
		}
	}

private:
	static size_type size_max(size_type x, size_type y) { return x > y ? x : y; }
	static size_type calc_aligned_capacity(size_type capacity) { return size_max(8, nextpow2(capacity * 2)); } // the one use of oCore/bit.h

	template<typename T> static void          init_array  (T* a, size_type n)                  { internal_init_array(a, n, std::is_trivially_constructible<T>()); }
	template<typename T> static void internal_init_array  (T* a, size_type n, std::true_type)  { memset(a, 0, sizeof(T) * n); }
	template<typename T> static void internal_init_array  (T* a, size_type n, std::false_type) { val_type* end = a + n; while (a < end) { ::new (a) T(); a++; } }
	template<typename T> static void          deinit_array(T* a, size_type n)                  { internal_deinit_array(a, n, std::is_trivially_destructible<T>()); }
	template<typename T> static void internal_deinit_array(T* a, size_type n, std::true_type ) {}
	template<typename T> static void internal_deinit_array(T* a, size_type n, std::false_type) { val_type* end = a + n; while (a < end) { a->~T(); a++; } }

	key_type* keys_;
	val_type* vals_;
	size_type size_;
	size_type wrap_;
};

}
