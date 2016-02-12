// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Macros to reduce the boilerplate associated with custom std::allocators

#pragma once
#include <oMemory/allocate.h>
#include <memory.h> // malloc/free

#define oDEFINE_STD_ALLOCATOR_BOILERPLATE(ClassName) \
	typedef size_t size_type; typedef ptrdiff_t difference_type; typedef T value_type; typedef T* pointer; typedef T const* const_pointer; typedef T& reference; typedef T const& const_reference; \
	template<typename U> struct rebind { typedef ClassName<U> other; }; \
	pointer address(reference p) const { return &p; } \
	const_pointer address(const_reference p) const { return &p; } \
	size_type max_size() const { return static_cast<size_type>(~0) / sizeof(value_type); } \
	void construct(pointer p) { ::new (static_cast<void*>(p)) T(); } \
	void construct(pointer p, const T& val) { ::new (static_cast<void*>(p)) T(val); } \
	void destroy(pointer p) { p->~T(); }

#define oDEFINE_STD_ALLOCATOR_VOID_INSTANTIATION(ClassName) \
	template <> class ClassName<void> \
	{ public: typedef void* pointer; typedef const void* const_pointer; typedef void value_type; \
	template <class U> struct rebind { typedef ClassName<U> other; }; \
	};

#define oDEFINE_STD_ALLOCATOR_OPERATOR_EQUAL(ClassName) \
	template<typename T, typename U> bool operator!=(ClassName<T> const& a, ClassName<U> const& b) { return !(a == b); } \
	template<typename T, typename U> bool operator==(ClassName<T> const& a, ClassName<U> const& b)

// As an example of this boilerplate usage, and as a semi-useful class itself,
// here is a std::allocator that takes user functions of the same signature as 
// malloc and free to do allocations in STL containers. Although this is 
// somewhat naughty in that state (the two function pointers) is retained, all 
// common STL implementations pass things through, and all copy and equality 
// semantics are enforced, so this works properly in all but the most contrived 
// cases.

template<typename T> struct std_user_allocator
{
	oDEFINE_STD_ALLOCATOR_BOILERPLATE(std_user_allocator)
	std_user_allocator() : user_alloc(malloc), user_dealloc(free) {}
	std_user_allocator(void* (*alloc)(size_t size), void (*dealloc)(void* p)) : user_alloc(alloc), user_dealloc(dealloc) {}
	template<typename U> std_user_allocator(std_user_allocator<U> const& other) : user_alloc(other.user_alloc), user_dealloc(other.user_dealloc) {}
	inline pointer allocate(size_type count, const_pointer hint = 0) { return static_cast<pointer>(user_alloc(sizeof(T) * count)); }
	inline void deallocate(pointer p, size_type count) { user_dealloc(p); }
	inline const std_user_allocator& operator=(const std_user_allocator& that) { user_alloc = that.user_alloc; user_dealloc = that.user_dealloc; return *this; }
	void* (*user_alloc)(size_t size);
	void (*user_dealloc)(void* p);
};

oDEFINE_STD_ALLOCATOR_VOID_INSTANTIATION(std_user_allocator)
oDEFINE_STD_ALLOCATOR_OPERATOR_EQUAL(std_user_allocator) { return a.user_alloc == b.user_alloc && a.user_dealloc == b.user_dealloc; }

// A ready-to-use wrapper for class allocator
template<typename T> struct ouro_std_allocator
{
	oDEFINE_STD_ALLOCATOR_BOILERPLATE(ouro_std_allocator)
	ouro_std_allocator() : alloc_(ouro::default_allocator), label_("ouro_std_allocator") {}
	ouro_std_allocator(const ouro::allocator& alloc, const char* label) : alloc_(alloc), label_(label) {}
	template<typename U> ouro_std_allocator(ouro_std_allocator<U> const& other) : alloc_(other.alloc_) {}
	inline pointer allocate(size_type count, const_pointer hint = 0) { return static_cast<pointer>(alloc_.allocate(sizeof(T) * count, label_)); }
	inline void deallocate(pointer p, size_type count) { alloc_.deallocate(p); }
	inline const ouro_std_allocator& operator=(const ouro_std_allocator& that) { alloc_ = that.alloc_; return *this; }
	ouro::allocator alloc_;
	const char* label_;
};

oDEFINE_STD_ALLOCATOR_VOID_INSTANTIATION(ouro_std_allocator)
oDEFINE_STD_ALLOCATOR_OPERATOR_EQUAL(ouro_std_allocator) { return a.alloc_ == b.alloc_; }

