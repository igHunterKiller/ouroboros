// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Use flexible array member pattern: allocate a large buffer and cast
// it to this class to enable management of an add-only list.

#pragma once
#include <cstdint>
#include <atomic>

namespace ouro {

struct flexible_array_bytes_t {};
struct flexible_array_count_t {};

template<bool use_atomic>
class flexible_array_base_t
{
protected:
	static const bool is_atomic = false;

	flexible_array_base_t(uint32_t capacity = 0, flexible_array_base_t* next = nullptr) : next_(next), size_(0), capacity_(capacity) {}
	flexible_array_base_t* next_;
  uint32_t size_;
	uint32_t capacity_;
};

template<>
class flexible_array_base_t<true>
{
	static_assert(sizeof(std::atomic_uint) == sizeof(uint32_t), "this class assumes sizeof(std::atomic_uint) == sizeof(uint32_t)");
protected:
	static const bool is_atomic = true;

	flexible_array_base_t(uint32_t capacity = 0, flexible_array_base_t* next = nullptr) : next_(next), capacity_(capacity) { size_ = 0; }
	flexible_array_base_t* next_;
  std::atomic_uint size_;
	uint32_t capacity_;
};

template<typename T, bool use_atomic = false>
class flexible_array_t : flexible_array_base_t<use_atomic>
{
public:
	typedef T data_type;
	static const bool is_atomic = flexible_array_base_t<use_atomic>::is_atomic;


	// == ctors ==

	flexible_array_t() : flexible_array_base_t() {}
	flexible_array_t(const flexible_array_bytes_t&, uint32_t capacity, flexible_array_t* next = nullptr) : flexible_array_base_t(capacity - sizeof(flexible_array_t) + sizeof(T), next) {}
	flexible_array_t(const flexible_array_count_t&, uint32_t capacity, flexible_array_t* next = nullptr) : flexible_array_base_t(capacity, next) {}
	flexible_array_t& operator=(flexible_array_t&& that)
	{
		if (this != &that)
		{
			next_     = that.next_;     that.next_ = nullptr;
			size_.store(that.size_);    that.size_.store(0);
			capacity_ = that.capacity_; that.capacity_ = 0;
		}
		return *this;
	}

	// == non-concurrent apis ==

	void initialize(const flexible_array_bytes_t& b, uint32_t capacity, flexible_array_t* next = nullptr) { *this = std::move(flexible_array_t(b, capacity, next)); }
	void initialize(const flexible_array_count_t& c, uint32_t capacity, flexible_array_t* next = nullptr) { *this = std::move(flexible_array_t(c, capacity, next)); }

	void* deinitialize() { clear(); return this; }

	const flexible_array_t* next() const { return next_; }
	void next(flexible_array_t* next) { next_ = next; }

	T* data() { return data_; }
	const T* data() const { return data_; }

	const T& operator[](int i) const { return data_[i]; }
	T& operator[](int i) { return data_[i]; }

	template<typename visitor_t>
	void visit(visitor_t visitor, void* user)
	{
		const uint32_t n = size_;
		for (uint32_t i = 0; i < n; i++)
			visitor(data_[i], user);
	}


	// == concurrent apis (if use_atomic == true) ==

	uint32_t size() const { return size_; }
	uint32_t capacity() const { return capacity_; }
	bool full() const { return size_ >= capacity_; }
	bool empty() const { return size_ == 0; }

	T* add() { uint32_t i = size_++; return (i < capacity_) ? (data_ + i) : nullptr; }
	void resize(uint32_t new_size) { size_ = new_size; }
	void clear() { size_ = 0; }

private:
	T data_[1];
};

}
