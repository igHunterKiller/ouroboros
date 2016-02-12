// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// binary large object (blob) that includes a pointer, its size and a
// means to free the memory. This is similar to std::unique_ptr, but 
// doesn't enforce the .get() pattern.

#pragma once

#include <utility>

namespace ouro {

class blob
{
public:
	// function signature for freeing memory when this object goes out of scope
	typedef void (*deleter_fn)(void* ptr);

	blob()                                               : pointer_(nullptr),       size_(0),          deleter_(nullptr) {}
	blob(void* pointer, size_t size, deleter_fn deleter) : pointer_(pointer),       size_(size) ,      deleter_(deleter) {}
	blob(blob&& that)                                    : pointer_(that.pointer_), size_(that.size_), deleter_(that.deleter_) { that.pointer_ = nullptr; that.size_ = 0; that.deleter_ = nullptr; }
	~blob()                                                                                                                    { if (pointer_ && deleter_) deleter_(pointer_); }

	blob& operator=(blob&& that)
	{
		if (this != &that)
		{
			if (pointer_ && deleter_) deleter_(pointer_);
			pointer_ = that.pointer_; that.pointer_ = nullptr;
			size_    = that.size_;    that.size_ = 0;
			deleter_ = that.deleter_; that.deleter_ = nullptr;
		}
		return *this;
	}

	void swap(blob& that)
	{
		std::swap(pointer_, that.pointer_);
		std::swap(size_,    that.size_);
		std::swap(deleter_, that.deleter_);
	}

	// return the underlying pointer without deleting it and invalidate the instance
	void* release()                          { void* p = pointer_; pointer_ = nullptr; size_ = 0; deleter_ = nullptr; return p; }

	// returns a blob pointing to the same memory but with a null deleter
	// for when lifetime is managed separately from an api that takes a blob
	blob alias()                       const { return blob(pointer_, size_, nullptr); }
	operator bool()                    const { return !!pointer_; }
	template<typename T> operator T*() const { return static_cast<T*>(pointer_); }
	void* ptr()                              { return pointer_; }
	const void* ptr()                  const { return pointer_; }
	size_t size()                      const { return size_; }
	deleter_fn deleter()               const { return deleter_; }

private:
	blob(const blob&);/* = delete; */
	const blob& operator=(const blob&);/* = delete; */

  void* pointer_;
	size_t size_;
	deleter_fn deleter_;
};

}
