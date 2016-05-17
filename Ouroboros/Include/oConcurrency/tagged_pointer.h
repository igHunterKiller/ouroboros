// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A pointer that gives up some of its address space to protect against ABA 
// concurrency issues.

#pragma once
//#include <oArch/arch.h>
#include <oCore/assert.h>
#include <atomic>
#include <stdexcept>

#if o32BIT == 1
	#define oTAGGED_POINTER_ALIGNMENT 16
#else
	#define oTAGGED_POINTER_ALIGNMENT 8
#endif

namespace ouro {

enum tagged_ptr_marked_t { marked };

template<typename T>
class tagged_pointer
{
public:
	#if o32BIT == 1
		static const size_t tag_bits = 3;
		static const size_t tag_shift = 0;
		static const size_t mark_shift = 3;
	#else
		static const size_t tag_bits = 7;
		static const size_t tag_shift = (sizeof(void*) * 8) - tag_bits;
		static const size_t mark_shift = (sizeof(void*) * 8) - 1;
	#endif
	static const size_t tag_mask = ((size_t(1) << tag_bits) - size_t(1)) << tag_shift;
	static const size_t mark_mask = size_t(1) << mark_shift;
	static const size_t tag_mark_mask = tag_mask | mark_mask;

	tagged_pointer() : tag_and_pointer(0) {}
	tagged_pointer(const tagged_pointer& that) : tag_and_pointer((uintptr_t)that.tag_and_pointer) {}
	tagged_pointer(tagged_pointer&& that) : tag_and_pointer((uintptr_t)that.tag_and_pointer) { that.tag_and_pointer = 0; }
	const tagged_pointer<T>& operator=(const tagged_pointer<T>& that) { tag_and_pointer = (uintptr_t)that.tag_and_pointer; return *this; }
	tagged_pointer<T>& operator=(tagged_pointer<T>&& that) { tag_and_pointer = (uintptr_t)that.tag_and_pointer; that.tag_and_pointer = 0; return *this; }
	bool operator==(const tagged_pointer<T>& that) const { return tag_and_pointer == that.tag_and_pointer; }
	bool operator!=(const tagged_pointer<T>& that) const { return tag_and_pointer != that.tag_and_pointer; }

	tagged_pointer(void* ptr, size_t _tag)
	{
		#ifdef _DEBUG
		if (((uintptr_t)ptr & tag_mark_mask) != 0)
			oThrow(std::errc::invalid_argument, "tagged_pointer pointers must be aligned to 16-bytes allow room for the tag and mark");
		#endif
		tag_and_pointer = uintptr_t(ptr) | uintptr_t((_tag & tag_mask) << tag_shift);
	}

	tagged_pointer(void* ptr, size_t _tag, tagged_ptr_marked_t)
	{
		#ifdef _DEBUG
		if (((uintptr_t)ptr & tag_mark_mask) != 0)
			oThrow(std::errc::invalid_argument, "tagged_pointer pointers must be aligned to 16-bytes allow room for the tag and mark");
		#endif
		tag_and_pointer = uintptr_t(ptr) | uintptr_t((_tag & tag_mask) << tag_shift) | uintptr_t(mark_mask);
	}

	// similar to std::atomic_flag::test_and_flag(), this tests if the pointer is
	// marked and returns its prior state. This can be used to have multiple code
	// paths result in exactly one winner that can follow through on additional
	// execution.
	bool test_and_mark()
	{
		return (tag_and_pointer.fetch_or(mark_mask) & mark_mask) != 0;
	}

	size_t tag() const
	{
		return (tag_and_pointer & tag_mask) >> tag_shift;
	}
	
	T* ptr() const { return (T*)(tag_and_pointer & ~(tag_mark_mask)); }
	
	inline bool cas(tagged_pointer<T>& Old, const tagged_pointer<T>& New)
	{
		uintptr_t O = Old.tag_and_pointer;
		return tag_and_pointer.compare_exchange_strong(O, New.tag_and_pointer);
	}

	// convenience to do the common operation of cas'ing in a new value while 
	// incrementing the tag. This should fail if old is different, same with a 
	// different tag, or is marked.
	bool assign(tagged_pointer<T>& old, tagged_pointer<T>& New)
	{
		return cas(old, tagged_pointer<T>(New.ptr(), old.tag()+1));
	}

private:
	std::atomic<uintptr_t> tag_and_pointer;
};

}
