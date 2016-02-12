// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A simple intrusive smart ptr implementation. This requires the user to 
// define the following unqualified API:
// void ref_reference(T* p);
// void ref_release(T* p);
// p::operator bool() const // (as in if (p) {...} or if (p != 0) {...})
//
// IDIOM: A novel behavior of ref is that you can pass its address to a function
// to receive a value. This is used in the Microsoft-style factory pattern 
// bool CreateMyObject(MyObject** ppObject). In this style the address of a ref 
// to receive the new object is passed. Doing so will not release any prior 
// value in the ref since this circumvents reference counting since the majority 
// of the time this is done as an initialization step, but in the rare case 
// where a ref is recycled, explicitly set it to nullptr before reuse.
//
// NOTE: It is often desirable to skip stepping into these boilerplate 
// operations in debugging. To do this in MSVC9 open regedit.exe and navigate to
// Win32 HKEY_LOCAL_MACHINE\Software\Microsoft\VisualStudio\9.0\NativeDE\StepOver
// Win64 HKEY_LOCAL_MACHINE\Software\Wow6423Node\Microsoft\VisualStudio\9.0\NativeDE\StepOver
// There, add regular expressions to match ref (and any other code you might 
// want to step over).
// So add a new String value named "step over ref" with value "ref.*". To skip
// over std::vector code add "step over std::vector" with value "std\:\:vector".
// More here: http://blogs.msdn.com/b/andypennell/archive/2004/02/06/69004.aspx

#pragma once

namespace ouro {

template<class T> struct ref
{
	ref() : ptr(nullptr) {}
	~ref() { if (ptr) ref_release(ptr); ptr = nullptr; }

	ref(const ref<T>& that)
	{
		ptr = const_cast<T*>(that.c_ptr());
		if (ptr) ref_reference(ptr);
	}

	// Specify ref = false if initializing from a factory call that
	// returns a ref of 1.
	ref(const T* that, bool ref = true)
	{
		ptr = static_cast<T*>(const_cast<T*>(that));
		if (ptr && ref) ref_reference(ptr);
	}

	ref<T>& operator=(T* that) volatile
	{
		if (that) ref_reference(that);
		if (ptr) ref_release(ptr);
		ptr = that;
		return const_cast<ref<T>&>(*this);
	}
	
	ref<T>& operator=(const ref<T>& that) volatile
	{
		// const_cast is ok because we're casting away the constness of the ref and 
		// not the underlying type
		if (that) ref_reference(const_cast<T*>(that.c_ptr()));
		if (ptr) ref_release(ptr);
		ptr = const_cast<T*>(that.c_ptr());
		return const_cast<ref<T>&>(*this);
	}

	ref(ref<T>&& that) : ptr(that.ptr) { that.ptr = nullptr; }

	ref<T>& operator=(ref<T>&& that) volatile
	{
		if (ptr != that.ptr)
		{
			if (ptr) ref_release(const_cast<T*>(ptr));
			ptr = that.ptr;
			that.ptr = nullptr;
		}
		return const_cast<ref<T>&>(*this);
	}

	T* operator->() { return ptr; }
	const T* operator->() const { return ptr; }
	
	operator T*() { return ptr; }
	operator const T*() const { return ptr; }

	T* operator->() volatile { return ptr; }
	const T* operator->() const volatile { return ptr; }
	
	operator T*() volatile { return ptr; }
	operator const T*() const volatile { return ptr; }
	// This version of the operator should only be used for "create" style 
	// functions, and functions that "retrieve" ref's i.e. QueryInterface.
	T** operator &() { return &ptr; } 

	// The const variant only makes sense for "retrieve" style calls, but 
	// otherwise see comments above.
	const T** operator &() const { return const_cast<const T**>(&ptr); } 
	
	T* c_ptr() { return ptr; }
	const T* c_ptr() const { return ptr; }

	T* c_ptr() volatile { return ptr; }
	const T* c_ptr() const volatile { return ptr; }

private:
	T* ptr;
};

}
