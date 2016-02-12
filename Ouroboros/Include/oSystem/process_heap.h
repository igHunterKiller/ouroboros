// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This object represents a memory heap that is independent of and persistent 
// across modules loaded into a process. This is useful for implementing objects 
// that require process-wide scope and need to retain their memory independent 
// of dynamic modules that might come and go or cause duplicate global static 
// objects such as singletons.

#pragma once
#include <oMemory/std_allocator.h>
#include <functional>

namespace ouro { namespace process_heap {

enum scope
{
	per_process,
	per_thread,
};

enum tracking
{
	none, // the allocation is not reported as a leak
	leak_tracked, // if still allocated, this is reported
	garbage_collected, // process_heap calls deallocate on object
};

// This ensures the process heap has been initialized
void ensure_initialized();

// Allocate memory from the process-global heap directly.
void* allocate(size_t _Size, size_t _Align = 64);

// Deallocate memory allocated from allocate() or find_or_allocate(). No 
// matter how many times find_or_allocate evaluates to a pre-existing pointer, 
// this deallocate will free the memory, potentially leaving dangling pointers. 
// It is up to the user to retain control of when the memory actually gets 
// freed. If a destructor was specified in find_or_allocate then it is called 
// before the actual freeing of the memory.
void deallocate(void* _Pointer);

// Call this at the end of a thread to free all thread-local memory.
void exit_thread();

// If the specified named allocation has already been allocated, this will fill 
// _pPointer with that pointer and return false. If the name is unrecognized or 
// in a different scope a new allocation will be made, the specified 
// _PlacementConstructor will be run and this will return true. Use deallocate 
// to free the memory, at which time the specified _Destructor will be run first 
// to finalize the object. A call to exit_thread() will deallocate all 
// allocations for its calling thread, so for per_thread allocations explicit
// calls to deallocate is not necessary if exit_thread is called.
bool find_or_allocate(size_t _Size
	, const char* _Name
	, scope _Scope
	, tracking _Tracking
	, const std::function<void(void* _Pointer)>& _PlacementConstructor
	, const std::function<void(void* _Pointer)>& _Destructor
	, void** _pPointer);

template<typename T>
bool find_or_allocate(
	const char* _Name
	, scope _Scope
	, tracking _Tracking
	, const std::function<void(void* _Pointer)>& _PlacementConstructor
	, const std::function<void(void* _Pointer)>& _Destructor
	, T** _pPointer)
{	return find_or_allocate(sizeof(T), _Name, _Scope, _Tracking, _PlacementConstructor, _Destructor, (void**)_pPointer); }

// Returns true if the specified allocation already exists and *_pPointer is 
// valid and false if the allocation does not exist.
bool find(const char* _Name, scope _Scope, void** _pPointer);

// This should be used for singleton members to ensure allocations are not 
// falsely detected as leaks.
template<typename T> struct std_allocator
{
	// Below we'll use a hash, but we want all its allocations to come out of the
	// specific process heap.

	oDEFINE_STD_ALLOCATOR_BOILERPLATE(std_allocator)
	std_allocator() {}
	template<typename U> std_allocator(std_allocator<U> const& other) {}
	inline pointer allocate(size_type count, const_pointer hint = 0) { return static_cast<pointer>(process_heap::allocate(sizeof(T) * count)); }
	inline void deallocate(pointer p, size_type count) { process_heap::deallocate(p); }
	inline const std_allocator& operator=(const std_allocator& other) {}
};
oDEFINE_STD_ALLOCATOR_VOID_INSTANTIATION(std_allocator)

}}

oDEFINE_STD_ALLOCATOR_OPERATOR_EQUAL(ouro::process_heap::std_allocator) { return true; }

// Convenience macro for defining a member function that acts as a singleton 
// instantiator that will be singular throughout the entire process, even with 
// DLLs.

#define oDEFINE_PROCESS_SINGLETON__(_APIName, _StrName, _ClassName) \
	_ClassName& _ClassName::_APIName() \
	{	static _ClassName* sInstance = nullptr; \
		if (!sInstance) \
		{	process_heap::find_or_allocate(_StrName, process_heap::per_process, process_heap::garbage_collected \
				, [=](void* _pMemory) { new (_pMemory) _ClassName(); } \
				, [=](void* _pMemory) { ((_ClassName*)_pMemory)->~_ClassName(); } \
				, &sInstance); \
		} \
		return *sInstance; \
	}

#define oDEFINE_PROCESS_SINGLETON(_StrName, _ClassName) oDEFINE_PROCESS_SINGLETON__(singleton, _StrName, _ClassName)
