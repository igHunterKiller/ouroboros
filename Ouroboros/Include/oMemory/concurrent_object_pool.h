// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// O(1) fine-grained concurrent typed block allocator: uses space inside free 
// blocks to maintain freelist.

#pragma once
#include <oMemory/concurrent_pool.h>

namespace ouro {

template<typename T>
class concurrent_object_pool : public concurrent_pool
{
public:
	typedef concurrent_pool::index_type index_type;
	typedef concurrent_pool::size_type size_type;
	typedef T value_type;

	static size_type calc_size(size_type capacity) { return concurrent_pool::calc_size(capacity, sizeof(T)); }
	static size_type calc_capacity(size_type bytes) { return concurrent_pool::calc_capacity(bytes, sizeof(T)); }

	
	// === non-concurrent api ===

	concurrent_object_pool() {}
	concurrent_object_pool(concurrent_object_pool&& that) : concurrent_pool(std::move((concurrent_pool&&)that)) {}
	concurrent_object_pool(void* memory, size_type bytes) : concurrent_pool(memory, bytes, sizeof(T)) {}
	~concurrent_object_pool() { ((concurrent_pool*)this)->~concurrent_pool(); }
	concurrent_object_pool& operator=(concurrent_object_pool&& that) { return (concurrent_object_pool&)concurrent_pool::operator=(std::move((concurrent_pool&&)that)); }

  void initialize(void* arena, size_type bytes) { concurrent_pool::initialize(arena, bytes, sizeof(T)); }


	// === concurrent api ===

	T* create() { void* p = allocate(); return p ? new (p) T() : nullptr; }
	
	template<typename A> 
	T* create(A&& that) { void* p = allocate(); return p ? new (p) T(std::move(that)) : nullptr; }
	
	template<typename A> 
	T* create(const A& a) { void* p = allocate(); return p ? new (p) T(a) : nullptr; }
	
	template<typename A, typename B>
	T* create(const A& a, const B& b) { void* p = allocate(); return p ? new (p) T(a,b) : nullptr; }
	
	template<typename A, typename B, typename C>
	T* create(const A& a, const B& b, const C& c) { void* p = allocate(); return p ? new (p) T(a,b,c) : nullptr; }
	
	template<typename A, typename B, typename C, typename D>
	T* create(const A& a, const B& b, const C& c, const D& d) { void* p = allocate(); return p ? new (p) T(a,b,c,d) : nullptr; }

	void destroy(T* ptr) { ptr->T::~T(); deallocate(ptr); }

	T* typed_pointer(index_type index) const { return (T*)concurrent_pool::pointer(index); }

private:
	concurrent_object_pool(const concurrent_object_pool&); /* = delete; */
	const concurrent_object_pool& operator=(const concurrent_object_pool&); /* = delete; */
};

}
