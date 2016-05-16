// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A concurrent stack that assumes a struct with a next pointer. No memory 
// management is done by the class itself.

#pragma once
#include <oArch/compiler.h>
#include <oCore/assert.h>
#include <oConcurrency/tagged_pointer.h>
#include <atomic>
#include <cstdint>
#include <stdexcept>

namespace ouro {

template<typename T>
class concurrent_stack
{
public:
	typedef uint32_t size_type;
	typedef T value_type;
	typedef value_type* pointer;

	
	// === non-concurrent api ===

	// initialized to a valid empty stack
	concurrent_stack();
	concurrent_stack(concurrent_stack&& that);
	~concurrent_stack();
	concurrent_stack& operator=(concurrent_stack&& that);

	// returns the top of the stack without removing the item from the stack
	pointer peek() const;

	// walks through the stack and counts the entries. Only use this for 
	// debugging/confirmation during times of no push/pops
	size_type size();


	// === concurrent api ===

	// push an element onto the stack
	void push(pointer element);

	// if there are no elements this return nullptr
	pointer pop();

	// if there are no elements this returns false, else it fills the address
	// with a pointer to the element popped.
	bool pop(pointer* element);

	// returns true if no elements are in the stack
	bool empty() const;

	// returns the head that is thus a linked list of all items that were in the 
	// stack, leaving the stack empty.
	pointer pop_all();

private:

	alignas(oCACHE_LINE_SIZE) tagged_pointer<T> head;

	concurrent_stack(const concurrent_stack&); /* = delete; */
	const concurrent_stack& operator=(const concurrent_stack&); /* = delete; */
};
static_assert(sizeof(concurrent_stack<int>) == oCACHE_LINE_SIZE, "size mismatch");

template<typename T>
concurrent_stack<T>::concurrent_stack()
{
}

template<typename T>
concurrent_stack<T>::concurrent_stack(concurrent_stack&& that)
{
	head = std::move(that.head);
}

template<typename T>
concurrent_stack<T>::~concurrent_stack()
{
	oAssert(empty(), "concurrent_stack not empty");
}

template<typename T>
typename concurrent_stack<T>& concurrent_stack<T>::operator=(concurrent_stack&& that)
{
	if (this != &that)
		head = std::move(that.head);
	return *this;
}

template<typename T>
void concurrent_stack<T>::push(pointer element)
{
	tagged_pointer<T> n, o(head); 
	do 
	{	o = head;
		element->next = reinterpret_cast<pointer>(o.ptr());
		n = tagged_pointer<T>(element, o.tag() + 1);
	} while (!head.cas(o, n));
}

template<typename T>
typename concurrent_stack<T>::pointer concurrent_stack<T>::peek() const
{
	tagged_pointer<T> o(head);
	return reinterpret_cast<pointer>(o.ptr());
}

template<typename T>
typename concurrent_stack<T>::pointer concurrent_stack<T>::pop()
{
	tagged_pointer<T> n, o(head); 
	pointer element = nullptr;
	do 
	{	o = head;
		if (!o.ptr())
			return nullptr;
		element = reinterpret_cast<pointer>(o.ptr());
		n = tagged_pointer<T>(element->next, o.tag() + 1);
	} while (!head.cas(o, n));
	element->next = nullptr;
	return element;
}

template<typename T>
bool concurrent_stack<T>::pop(pointer* element)
{
	tagged_pointer<T> n, o(head); 
	do 
	{	o = head;
		if (!o.ptr())
			return false;
		*element = reinterpret_cast<pointer>(o.ptr());
		n = tagged_pointer<T>((*element)->next, o.tag() + 1);
	} while (!head.cas(o, n));
	(*element)->next = nullptr;
	return true;
}

template<typename T>
bool concurrent_stack<T>::empty() const
{
	tagged_pointer<T> o(head);
	return !o.ptr();
}

template<typename T>
typename concurrent_stack<T>::pointer concurrent_stack<T>::pop_all()
{
	pointer element = nullptr;
	tagged_pointer<T> n, o(head);
	do 
	{	o = head;		
		element = reinterpret_cast<pointer>(o.ptr());
		n = tagged_pointer<T>(nullptr, o.tag() + 1);
	} while (!head.cas(o, n));
	return element;
}

template<typename T>
typename concurrent_stack<T>::size_type concurrent_stack<T>::size()
{
	pointer p = peek();
	size_type n = 0;
	while (p)
	{
		n++;
		p = p->next;
	}
	return n;
}

}
