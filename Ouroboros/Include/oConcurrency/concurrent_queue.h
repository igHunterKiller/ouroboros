// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Fine-grained concurrent FIFO queue based on:
// http://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html
// Use a concurrent fixed block allocator for best performance. This also 
// ensures concurrency of non-trivial destructors.

#pragma once
//#include <oArch/arch.h>
//#include <oArch/arch.h>
#include <oConcurrency/tagged_pointer.h>
#include <oMemory/allocate.h>
#include <oMemory/concurrent_object_pool.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace ouro {

template<typename T, bool has_trivial_dtor = std::is_trivially_destructible<T>::value>
struct concurrent_queue_node {};

template<typename T>
struct concurrent_queue_node<T, true>
{
  typedef T value_type;
  typedef tagged_pointer<concurrent_queue_node<value_type, true>> pointer_type;
  
  concurrent_queue_node(const value_type& v) : next(nullptr, 0), value(v) {}
  concurrent_queue_node(value_type&& v) : next(nullptr, 0), value(std::move(v)) {}

  pointer_type next;
  value_type value;
};

template<typename T>
struct concurrent_queue_node<T, false>
{
  typedef T value_type;
  typedef tagged_pointer<concurrent_queue_node<value_type, false>> pointer_type;
  
  concurrent_queue_node(const value_type& v) : next(nullptr, 0), value(v) { flag.clear(); }
  concurrent_queue_node(value_type&& v) : next(nullptr, 0), value(std::move(v)) { flag.clear(); }

  pointer_type next;
  value_type value;
  
	// A two-step trivial ref count. In try_pop, the race condition described in 
	// the original paper for non-trivial destructors is addressed by flagging 
	// which of the two conditions/code paths should be allowed to free the 
	// memory, so calling this query is destructive (thus the non-constness).
	bool should_deallocate() { return flag.test_and_set(); }

private:
	std::atomic_flag flag;
};

template<typename T>
class concurrent_queue
{
public:
	typedef uint32_t size_type;
	typedef T value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
  typedef concurrent_queue_node<T> node_type;
  typedef typename node_type::pointer_type pointer_type;

	static const size_type default_capacity = 65536;

	static size_type calc_size(size_type capacity);


	// non-concurrent api

	concurrent_queue(size_type capacity = default_capacity, const char* label = "concurrent_queue", const allocator& a = default_allocator);
	~concurrent_queue();

	// initializes the queue with memory allocated from allocator
	void initialize(size_type capacity, const char* label = "concurrent_queue", const allocator& a = default_allocator);

	// use calc_size() to determine memory size
	void initialize(void* memory, size_type capacity);

	// deinitializes the queue and returns the memory passed to initialize()
	void* deinitialize();

	// Walks the nodes in a non-concurrent manner and returns the count (not 
	// including the sentinel node). This should be used only for debugging.
	size_type size() const;


	// concurrent api

	// Push an element into the queue.
	void push(const_reference val);
	void push(value_type&& val);

	// Returns false if the queue is empty
	bool try_pop(reference val);

	// Spins until an element can be popped from the queue
	void pop(reference val);

	// Spins until the queue is empty
	void clear();

	// Returns true if no elements are in the queue
	bool empty() const;

private:
	// alignment is required so that pointers to node_type's are at least 8-bytes.
	// This allows tagged_pointer to use the bottom 3-bits for its tag.
	
	alignas(oCACHE_LINE_SIZE) pointer_type head;
	alignas(oCACHE_LINE_SIZE) pointer_type tail;
	concurrent_object_pool<node_type> pool;
	allocator alloc;

	node_type* internal_construct_sentinel(std::false_type);
	node_type* internal_construct_sentinel(std::true_type);

	void internal_push(node_type* n);

	bool internal_try_pop(reference val, std::false_type);
	bool internal_try_pop(reference val, std::true_type);
};

template<typename T>
typename concurrent_queue<T>::size_type concurrent_queue<T>::calc_size(size_type capacity)
{
	return concurrent_object_pool<node_type>::calc_size(capacity);
}

template<typename T>
typename concurrent_queue<T>::node_type* concurrent_queue<T>::internal_construct_sentinel(std::false_type)
{
	node_type* n = pool.create(value_type());
	n->should_deallocate(); // mark for immediate since this won't be popped
	return n;
}

template<typename T>
typename concurrent_queue<T>::node_type* concurrent_queue<T>::internal_construct_sentinel(std::true_type)
{
	return pool.create(value_type());
}

template<typename T>
concurrent_queue<T>::concurrent_queue(size_type capacity, const char* label, const allocator& a)
{
	initialize(capacity, label, a);
}

template<typename T>
concurrent_queue<T>::~concurrent_queue()
{
	deinitialize();
}

template<typename T>
void concurrent_queue<T>::initialize(size_type capacity, const char* label, const allocator& a)
{
	alloc = a;
	auto bytes = calc_size(capacity);
	void* mem = alloc.allocate(bytes, label, memory_alignment::cacheline);
	pool.initialize(mem, bytes);
	node_type* n = internal_construct_sentinel(std::is_trivially_destructible<T>());
	head = tail = pointer_type(n, 0);
}

template<typename T>
void concurrent_queue<T>::initialize(void* memory, size_type capacity)
{
	alloc = noop_allocator;
	pool.initialize(memory, pool.calc_size(capacity));
	node_type* n = internal_construct_sentinel(std::is_trivially_destructible<T>());
	head = tail = pointer_type(n, 0);
}

template<typename T>
void* concurrent_queue<T>::deinitialize()
{
	if (!empty())
		throw std::length_error("container not empty");
	node_type* n = head.ptr();
	head = tail = pointer_type(nullptr, 0);
	pool.destroy(n); // sentinel already destroyed, so deallocate directly here
	void* mem = pool.deinitialize();
	alloc.deallocate(mem);
	mem = alloc == noop_allocator ? nullptr : mem;
	alloc = noop_allocator;
	return mem;
}

template<typename T>
typename concurrent_queue<T>::size_type concurrent_queue<T>::size() const
{
	size_type n = 0;
	auto p = head.ptr()->next.ptr();
	while (p)
	{
		n++;
		p = p->next.ptr();
	}
	return n;
}

template<typename T>
void concurrent_queue<T>::internal_push(node_type* n)
{
	if (!n)
		throw std::bad_alloc();
	pointer_type t, next;
	for (;;)
	{
		t = tail;
		next = t.ptr()->next;
		if (t == tail)
		{
			if (!next.ptr())
			{
				if (tail.ptr()->next.cas(next, pointer_type(n, next.tag()+1)))
					break;
			}

			else
				tail.cas(t, pointer_type(next.ptr(), t.tag()+1));
		}
	}

	tail.cas(t, pointer_type(n, t.tag()+1));
}

template<typename T>
void concurrent_queue<T>::push(const_reference val)
{
	internal_push(pool.create(val));
}

template<typename T>
void concurrent_queue<T>::push(value_type&& val)
{
	internal_push(pool.create(std::move(val)));
}

template<typename T>
bool concurrent_queue<T>::internal_try_pop(reference val, std::false_type)
{
	pointer_type h, t, next;
	for (;;)
	{
		h = head;
		t = tail;
		next = h.ptr()->next;
		auto nptr = next.ptr();
		if (h == head)
		{
			if (h.ptr() == t.ptr())
			{
				if (!nptr) return false;
				tail.cas(t, pointer_type(nptr, t.tag()+1));
			}

			else
			{
				if (head.cas(h, pointer_type(nptr, h.tag()+1)))
				{
					// Yes, the paper says the assignment should be outside the CAS,
					// but we've worked around that so we can also call the destructor
					// here protected by the above CAS by flagging when the destructor
					// is done and the memory can truly be reclaimed, so the 
					// should_deallocate() calls have been added to either clean up the
					// memory immediately now that the CAS has made next the dummy head,
					// or clean it up lazily later at the bottom. Either way, do it only 
					// once.
					val = std::move(nptr->value);
					nptr->value.~T();
					if (nptr->should_deallocate())
						pool.destroy(nptr);
					break;
				}
			}
		}
	}

	auto p = h.ptr();
	if (p->should_deallocate())
		pool.destroy(p); // dtor called explicitly above so just deallocate
	
	return true;
}

template<typename T>
bool concurrent_queue<T>::internal_try_pop(reference val, std::true_type)
{
	pointer_type h, t, next;
	for (;;)
	{
		h = head;
		t = tail;
		next = h.ptr()->next;
		auto nptr = next.ptr();
		if (h == head)
		{
			if (h.ptr() == t.ptr())
			{
				if (!nptr) return false;
				tail.cas(t, pointer_type(nptr, t.tag()+1));
			}

			else
			{
				val = std::move(nptr->value);
				if (head.cas(h, pointer_type(nptr, h.tag()+1)))
					break;
			}
		}
	}

	pool.destroy(h.ptr()); // dtor called explicitly above so just deallocate
	return true;
}

template<typename T>
bool concurrent_queue<T>::try_pop(reference val)
{
	return internal_try_pop(val, std::is_trivially_destructible<T>());
}
	
template<typename T>
void concurrent_queue<T>::pop(reference val)
{
	while (!try_pop(val));
}

template<typename T>
void concurrent_queue<T>::clear()
{
	value_type e;
	while (try_pop(e));
}

template<typename T>
bool concurrent_queue<T>::empty() const
{
	return head == tail;
}

}
