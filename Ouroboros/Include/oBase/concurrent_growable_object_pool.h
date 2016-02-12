// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#ifndef oBase_concurrent_growable_object_pool_h
#define oBase_concurrent_growable_object_pool_h

#include <oBase/callable.h>
#include <oBase/concurrent_growable_pool.h>

// emulate variadic macros for passing various parameters to create()
#define oCONCURRENT_GROWABLE_OBJECT_POOL_CREATE_N(n) \
	template<oCALLABLE_CONCAT(oARG_TYPENAMES,n)> \
	T* create(oCALLABLE_CONCAT(oARG_DECL,n)) { void* p = allocate_pointer(); return p ? new (p) T(oCALLABLE_CONCAT(oARG_PASS,n)) : nullptr; }
#define oCONCURRENT_GROWABLE_OBJECT_POOL_CREATE_DESTROY \
	oCALLABLE_PROPAGATE_SKIP0(oCONCURRENT_GROWABLE_OBJECT_POOL_CREATE_N) \
	T* create() { void* p = allocate_pointer(); return p ? new (p) T() : nullptr; } \
	void destroy(T* _Pointer) { _Pointer->T::~T(); deallocate(_Pointer); }

namespace ouro {

template<typename T>
class concurrent_growable_object_pool : public concurrent_growable_pool
{
public:
	typedef concurrent_pool::size_type size_type;
	typedef T value_type;


	// non-concurrent api

	concurrent_growable_object_pool() {}
	concurrent_growable_object_pool(concurrent_growable_object_pool&& _That) : concurrent_growable_pool(std::move((concurrent_growable_pool&&)_That)) {}
	concurrent_growable_object_pool(size_type capacity) : concurrent_growable_pool(sizeof(T), capacity, oALIGNOF(value_type)) {}
	concurrent_growable_object_pool(size_type capacity, size_type alignment) : concurrent_growable_pool(sizeof(T), capacity, alignment) {}
	~concurrent_growable_object_pool() {}
	concurrent_growable_object_pool& operator=(concurrent_growable_object_pool&& _That) { return (concurrent_growable_object_pool&)concurrent_growable_pool::operator=(std::move((concurrent_growable_pool&&)_That)); }

	bool initialize(size_type capacity_per_chunk) { return concurrent_growable_pool::initialize(sizeof(T), capacity_per_chunk); }


	// concurrent api

	oCONCURRENT_GROWABLE_OBJECT_POOL_CREATE_DESTROY

private:
	concurrent_growable_object_pool(const concurrent_growable_object_pool&); /* = delete; */
	const concurrent_growable_object_pool& operator=(const concurrent_growable_object_pool&); /* = delete; */
};

}

#endif
