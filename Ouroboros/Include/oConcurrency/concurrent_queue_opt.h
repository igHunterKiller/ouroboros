// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Fine-grained concurrent FIFO queue based on:
// http://people.csail.mit.edu/edya/publications/OptimisticFIFOQueue-journal.pdf
// This is advertised as more efficient than concurrent_queue's algorithm because 
// it goes to great lengths to avoid atomic operations, optimistically assuming 
// contention does not happen and going back to fix any confusion if it does. 
// My benchmarks show marginal if any benefits. Use a concurrent block allocator
// for best performance.

#pragma once
//#include <oArch/arch.h>
#include <oConcurrency/tagged_pointer.h>
#include <oMemory/allocate.h>
#include <oMemory/concurrent_object_pool.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace ouro {

template<typename T>
struct alignas(oTAGGED_POINTER_ALIGNMENT) concurrent_queue_opt_node
{
  typedef T value_type;
  typedef tagged_pointer<concurrent_queue_opt_node<value_type>> pointer_type;

	concurrent_queue_opt_node(const value_type& val) : prev(nullptr, 0), next(nullptr, 0), value(val) {}
	concurrent_queue_opt_node(value_type&& val) : prev(nullptr, 0), next(nullptr, 0), value(std::move(val)) {}
	pointer_type prev;
	pointer_type next;
	value_type value;
};

template<typename T>
class concurrent_queue_opt
{
public:
	typedef uint32_t size_type;
	typedef T value_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
  typedef concurrent_queue_opt_node<T> node_type;
  typedef typename node_type::pointer_type pointer_type;

	static const size_type default_capacity = 65536;

	static size_type calc_size(size_type capacity);

	
	// non-concurrent api

	concurrent_queue_opt(size_type capacity = default_capacity, const char* label = "concurrent_queue_opt", const allocator& a = default_allocator);
	~concurrent_queue_opt();

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

	alignas(oCACHE_LINE_SIZE) pointer_type head;
	alignas(oCACHE_LINE_SIZE) pointer_type tail;
	concurrent_object_pool<node_type> pool;
	allocator alloc;

	void internal_push(node_type* n);
	void fix_list(pointer_type _tail, pointer_type _head);
};

template<typename T>
typename concurrent_queue_opt<T>::size_type concurrent_queue_opt<T>::calc_size(size_type capacity)
{
	return concurrent_object_pool<node_type>::calc_size(capacity);
}

template<typename T>
concurrent_queue_opt<T>::concurrent_queue_opt(size_type capacity, const char* label, const allocator& a)
{
	initialize(capacity, label, a);
}

template<typename T>
concurrent_queue_opt<T>::~concurrent_queue_opt()
{
	deinitialize();
}

template<typename T>
void concurrent_queue_opt<T>::initialize(size_type capacity, const char* label, const allocator& a)
{
	alloc = a;
	auto bytes = calc_size(capacity);
	void* mem = alloc.allocate(bytes, label, memory_alignment::cacheline);
	pool.initialize(mem, bytes);
	head = tail = pointer_type(pool.create(value_type()), 0);
}

template<typename T>
void concurrent_queue_opt<T>::initialize(void* memory, size_type capacity)
{
	alloc = noop_allocator;
	pool.initialize(memory, pool.calc_size(capacity));
	head = tail = pointer_type(pool.create(value_type()), 0);
}

template<typename T>
void* concurrent_queue_opt<T>::deinitialize()
{
	if (!empty())
		throw std::length_error("container not empty");
	node_type* n = head.ptr();
	head = tail = pointer_type(nullptr, 0);
	pool.destroy(n);
	void* mem = pool.deinitialize();
	alloc.deallocate(mem);
	mem = alloc == noop_allocator ? nullptr : mem;
	alloc = noop_allocator;
	return mem;
}

template<typename T>
typename concurrent_queue_opt<T>::size_type concurrent_queue_opt<T>::size() const
{
	size_type n = 0;
	auto p = head.ptr()->prev.ptr();
	while (p)
	{
		n++;
		p = p->prev.ptr();
	}
	return n;
}

template<typename T>
void concurrent_queue_opt<T>::internal_push(node_type* n)
{
	if (!n) throw std::bad_alloc();
	pointer_type t;
	for (;;)
	{
		t = tail;
		n->next = pointer_type(t.ptr(), t.tag()+1);
		if (tail.cas(t, pointer_type(n, t.tag()+1)))
		{
			// Seemingly during this time we have a new tail, but tail->next's prev 
			// pointer is still uninitialized (that happens below) but any time 
			// between the CAS and this next line a try_pop could try to deference
			// its firstNodePrev, which might be pointing to the same thing as t here.
			t.ptr()->prev = pointer_type(n, t.tag());
			break;
		}
	}
}

template<typename T>
void concurrent_queue_opt<T>::push(const_reference val)
{
	internal_push(pool.create(val));
}

template<typename T>
void concurrent_queue_opt<T>::push(value_type&& val)
{
	internal_push(pool.create(std::move(val)));
}

template<typename T>
void concurrent_queue_opt<T>::fix_list(pointer_type _tail, pointer_type _head)
{
	pointer_type curNode, curNodeNext;
	curNode = _tail;
	while ((_head == head) && (curNode != _head))
	{
		curNodeNext = curNode.ptr()->next;
		curNodeNext.ptr()->prev = pointer_type(curNode.ptr(), curNode.tag()-1);
		curNode = pointer_type(curNodeNext.ptr(), curNode.tag()-1);
	}
}

template<typename T>
bool concurrent_queue_opt<T>::try_pop(reference val)
{
	pointer_type t, h, firstNodePrev;
	for (;;)
	{
		h = head;
		t = tail;
		firstNodePrev = h.ptr()->prev;
		if (h == head)
		{
			if (t != h)
			{
				// Not in the original paper, but there is a race condition where 
				// push adds a node, but leaves node->next->prev uninitialized for a
				// tick. This only manifests too when firstNodePrev.tag == h.tag, which 
				// is also very rare. If they are not equal, fix_list fixes the issue 
				// (or at least takes long enough that things settle). So here ensure 
				// time is not wasted getting to the end-game only to try to dereference 
				// a nullptr.
				if (firstNodePrev.ptr() == nullptr)
					continue;

				if (firstNodePrev.tag() != h.tag())
				{
					fix_list(t, h);
					continue;
				}

				val = std::move(firstNodePrev.ptr()->value);
				if (head.cas(h, pointer_type(firstNodePrev.ptr(), h.tag()+1)))
				{
					pool.destroy(h.ptr());
					return true;
				}
			}

			else
				break;
		}
	}

	return false;
}

template<typename T>
void concurrent_queue_opt<T>::pop(reference val)
{
	while (!try_pop(val));
}

template<typename T>
void concurrent_queue_opt<T>::clear()
{
	value_type e;
	while (try_pop(e));
}

template<typename T>
bool concurrent_queue_opt<T>::empty() const
{
	return head == tail;
}

}
