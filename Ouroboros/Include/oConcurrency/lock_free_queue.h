// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Single-reader, single-writer queue that uses only memory barriers to protect
// concurrency.

#pragma once
#include <oConcurrency/concurrency.h>
#include <oMemory/allocate.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace ouro {

template<typename T>
class lock_free_queue
{
public:
	typedef uint32_t size_type;
	typedef T value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;

	// returns the number of bytes required to pass as memory to initialize().
	static size_type calc_size(size_type capacity);
	

	// === non-concurrent api ===

	// default ctor is uninitialized
	lock_free_queue();

	// capacity must be a power of two.
	lock_free_queue(size_type capacity, const char* label = "lock_free_queue", const allocator& a = default_allocator);
	~lock_free_queue();

	// capacity must be a power of two.
	void initialize(size_type capacity, const char* label = "lock_free_queue", const allocator& a = default_allocator);
	
	// capacity must be a power of two.
	void initialize(void* memory, size_type capacity);

	// returns memory passed to initialize or nullptr if allocator was specified
	void* deinitialize();

	// Returns the number of elements in the queue. Client code should not be 
	// reliant on this value and the API is included only for debugging and 
	// testing purposes. It is not concurrent.
	size_type size() const;


	// === concurrent api ===

	// Push an element into the queue.
	void push(const_reference val);
	
	// Returns false if the queue is empty, otherwise val is a valid element
	// from the head of the queue.
	bool try_pop(reference val);

	// Returns true if no elements are in the queue
	bool empty() const;

	// Returns the max number of elements that can be stored
	size_type capacity() const;
	
private:
	value_type* elements;
	size_type wrap_mask;
	volatile size_type read;
	volatile size_type write;
	allocator alloc;
};

template<typename T>
typename lock_free_queue<T>::size_type lock_free_queue<T>::calc_size(size_type capacity)
{
	return sizeof(T) * capacity;
}

template<typename T>
lock_free_queue<T>::lock_free_queue()
	: elements(nullptr)
	, wrap_mask(0)
	, read(0)
	, write(0)
	, alloc(noop_allocator)
{
}

template<typename T>
lock_free_queue<T>::lock_free_queue(size_type capacity, const char* label, const allocator& a)
{
	initialize(capacity, label, a);
}

template<typename T>
lock_free_queue<T>::~lock_free_queue()
{
	if (!empty())
		throw std::length_error("container not empty");

	if (deinitialize())
		throw std::invalid_argument("container not empty");
}

template<typename T>
void lock_free_queue<T>::initialize(size_type capacity, const char* label, const allocator& a)
{
	if (capacity & (capacity-1))
		throw std::invalid_argument("capacity must be a power of two");
	alloc = a;
	elements = alloc.allocate(calc_size(capacity), memory_alignment::align_default, label);
	read = write = 0;	
	wrap_mask = capacity - 1;
}

template<typename T>
void lock_free_queue<T>::initialize(void* memory, size_type capacity)
{
	if (capacity & (capacity-1))
		throw std::invalid_argument("capacity must be a power of two");
	elements = memory;
	read = write = 0;	
	wrap_mask = capacity - 1;
	alloc = noop_allocator;
}

template<typename T>
void* lock_free_queue<T>::deinitialize()
{
	void* mem = alloc == noop_allocator ? elements : nullptr;
	alloc.deallocate(elements);
	elements = nullptr;
	read = write = 0;
	wrap_mask = 0;
	alloc = noop_allocator;
	return mem;
}

template<typename T>
typename lock_free_queue<T>::size_type lock_free_queue<T>::size() const
{
	return ((write + wrap_mask) - read) & wrap_mask;
}

template<typename T>
void lock_free_queue<T>::push(const_reference val)
{
	size_type r = read;
	size_type w = write;

	if (((w+1) & wrap_mask) != r)
	{
		std::atomic_thread_fence(std::memory_order_seq_cst);
		elements[w++] = val;
		std::atomic_thread_fence(std::memory_order_seq_cst);
		write = w & wrap_mask;
	}

	else
		throw std::length_error("lock_free_queue cannot hold any more elements");
}

template<typename T>
bool lock_free_queue<T>::try_pop(reference val)
{
	bool popped = false;
	size_type r = read;
	size_type w = write;

	if (r != w)
	{
		std::atomic_thread_fence(std::memory_order_seq_cst);
		val = elements[r];
		elements[r++].~value_type();
		std::atomic_thread_fence(std::memory_order_seq_cst);
		read = r & wrap_mask;
		popped = true;
	}

	return popped;
}

template<typename T>
bool lock_free_queue<T>::empty() const
{
	return read == write;
}

template<typename T>
typename lock_free_queue<T>::size_type lock_free_queue<T>::capacity() const
{
	return wrap_mask ? (wrap_mask + 1) : 0;
}

}
