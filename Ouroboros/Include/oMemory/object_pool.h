// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) typed block allocator: uses space inside free blocks to maintain freelist.

#pragma once
#include <oMemory/pool.h>

namespace ouro {

template<typename T>
class object_pool : public pool
{
public:
	typedef pool::index_type index_type;
	typedef pool::size_type size_type;
	typedef T value_type;

	static size_type calc_size(size_type capacity) { return pool::calc_size(capacity, sizeof(T)); }
	static size_type calc_capacity(size_type bytes) { return pool::calc_capacity(bytes, sizeof(T)); }
	
	object_pool() {}
	object_pool(object_pool&& that) : pool(std::move((pool&&)that)) {}
	object_pool(void* memory, size_type bytes) : pool(memory, bytes, sizeof(T)) {}
	~object_pool() { ((pool*)this)->~pool(); }
	object_pool& operator=(object_pool&& that) { return (object_pool&)pool::operator=(std::move((pool&&)that)); }
	object_pool(const object_pool&) = delete;
	const object_pool& operator=(const object_pool&) = delete;
	
  void initialize(void* memory, size_type bytes) { pool::initialize(memory, bytes, sizeof(T)); }

	T* create() { void* p = allocate(); return p ? new (p) T() : nullptr; }
	
	template<typename A> 
	T* create(const A& a) { void* p = allocate(); return p ? new (p) T(a) : nullptr; }
	
	template<typename A, typename B>
	T* create(const A& a, const B& b) { void* p = allocate(); return p ? new (p) T(a,b) : nullptr; }
	
	template<typename A, typename B, typename C>
	T* create(const A& a, const B& b, const C& c) { void* p = allocate(); return p ? new (p) T(a,b,c) : nullptr; }
	
	template<typename A, typename B, typename C, typename D>
	T* create(const A& a, const B& b, const C& c, const D& d) { void* p = allocate(); return p ? new (p) T(a,b,c,d) : nullptr; }

	void destroy(T* ptr) { ptr->T::~T(); deallocate(ptr); }

	T* typed_pointer(index_type index) const { return (T*)pool::pointer(index); }
};

}
