// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A generic interface for allocating memory

#pragma once

#include <oCore/blob.h>
#include <cstdint>
#include <system_error>
#include <utility>

namespace ouro {

// _____________________________________________________________________________
// Error handling
	
enum class allocate_errc : uint32_t
{
	invalid,
	invalid_ptr,
	invalid_block_size,
  invalid_bookkeeping,
  invalid_capacity,
	out_of_memory,
	fragmented,
	corrupt,
	alignment,
  size_alignment,
	outstanding_allocations,
};

const std::error_category& allocate_category();

inline std::error_code make_error_code(allocate_errc err_code)           noexcept { return std::error_code(static_cast<int>(err_code), allocate_category()); }
inline std::error_condition make_error_condition(allocate_errc err_code) noexcept { return std::error_condition(static_cast<int>(err_code), allocate_category()); }

class allocate_error : public std::logic_error
{
public:
	allocate_error(allocate_errc err_code) 
		: logic_error(allocate_category().message(static_cast<int>(err_code))) {}
};

// _____________________________________________________________________________
// Definitions of standard memory configuration options

enum class memory_operation : uint8_t
{
	none,
	allocate,
	reallocate,
	deallocate,
	relocate,
	
	count, 
};

enum class memory_alignment : uint32_t
{
	align_default,
	align2,
	align4,
	align8,
	align16,
	align32,
	align64,
	align128,
	align256,
	align512,
	align1k,
	align2k,
	align4k,
	align8k,
	align16k,
	align32k,
	align64k,

	count,
	cacheline = align64,
	default_alignment = align16,
};

enum class memory_type : uint32_t
{
	cpu_writeback,
	cpu_writecombine,
	cpu_gpu_coherent,
	gpu_writecombine,
	gpu_readonly,
	gpu_on_chip,
	io_read_write,

	count,
};

class allocate_options
{
public:
	allocate_options()                                                                    : options(0)       {}
	allocate_options(uint32_t options)                                                    : options(options) {}
	allocate_options(const memory_alignment& a)                                           : options(0)       { alignment = a; }
	allocate_options(const memory_type& t, const memory_alignment& a)                     : options(0)       { type = t; alignment = a; }
	allocate_options(const memory_type& t, const memory_alignment& a, uint32_t category_)                    { type = t; alignment = a; category = category_; }
	operator uint32_t() const { return options; }

  size_t alignment_bytes() const { return static_cast<size_t>(size_t(1) << (alignment != memory_alignment::align_default ? (size_t)alignment : (size_t)memory_alignment::default_alignment)); }

	template<typename T> T align(const T& x)
	{
		auto align1 = alignment_bytes() - 1;
		return (T)(((size_t)x + align1) & ~align1);
	}

	template<typename T> bool aligned(const T& x)
	{
		return align(x) == x;
	}

private:
  union
  {
	  uint32_t options;
	  struct
	  {
		  memory_alignment alignment : 6; // memory_alignment
		  memory_type      type      : 4;
		  uint32_t         category  : 22; // passed through for user-markup
	  };
  };
};
static_assert(sizeof(allocate_options) == sizeof(uint32_t), "allocate_options size mismatch");

// _____________________________________________________________________________
// Standard ways of tracking and reporting memory

typedef void (*allocate_heap_walk_fn)(void* ptr, size_t bytes, int used, void* user);

struct allocation_stats
{
	allocation_stats()
		: pointer(nullptr)
		, label("unlabeled")
		, size(0)
		, options(0)
		, ordinal(0)
		, frame(0)
		, operation(memory_operation::none)
	{}

	void* pointer;
	const char* label;
	size_t size;
	allocate_options options;
	uint32_t ordinal;
	uint32_t frame;
	memory_operation operation;
};

struct allocator_stats
{
	allocator_stats()
		: allocated_bytes(0)
		, allocated_bytes_peak(0)
		, capacity_bytes(0)
		, num_allocations(0)
		, num_allocations_peak(0)
		, allocation_capacity(0)
		, largest_free_block_bytes(0)
		, num_free_blocks(0)
	{}

	size_t allocated_bytes;
	size_t allocated_bytes_peak;
	size_t capacity_bytes;
	size_t num_allocations;
	size_t num_allocations_peak;
	size_t allocation_capacity;
	size_t largest_free_block_bytes;
	size_t num_free_blocks;
};

// _____________________________________________________________________________
// Allocator definitions: callback functions, RAII pointer wrapper and a simple 
// allocator interface that can easily be passed around and retained by objects.

typedef void* (*allocate_fn)(size_t bytes, const char* label, const allocate_options& options);
typedef blob::deleter_fn deallocate_fn;
typedef void (*allocate_track_fn)(uint64_t allocator_id, const allocation_stats& stats);

// _____________________________________________________________________________
// Default implementations

void* default_allocate(size_t bytes, const char* label = "?", const allocate_options& options = allocate_options());
void default_deallocate(void* pointer);
void default_allocate_track(uint64_t allocation_id, const allocation_stats& stats);

void* noop_allocate(size_t bytes, const char* label = "?", const allocate_options& options = allocate_options());
void noop_deallocate(void* pointer);

// _____________________________________________________________________________
// The main allocator interface

class allocator
{
public:
	allocator() : allocate_(nullptr), deallocate_(nullptr) {}
	allocator(allocate_fn alloc, deallocate_fn dealloc) : allocate_(alloc), deallocate_(dealloc) {}
	
	operator bool() const { return allocate_ && deallocate_; }

	bool operator==(const allocator& that) const { return allocate_ == that.allocate_ && deallocate_ == that.deallocate_; }

  operator allocate_fn() const { return allocate_; }
  deallocate_fn deallocator() const { return deallocate_; }

  void* allocate(size_t bytes, const char* label = "?", const allocate_options& options = allocate_options()) const { return allocate_(bytes, label, options); }
  void deallocate(void* p) const { deallocate_(p); }

	blob scoped_allocate(size_t bytes, const char* label = "?", const allocate_options& options = allocate_options()) const { return blob(allocate(bytes, label, options), bytes, (blob::deleter_fn)deallocate_); }

	template<typename T>
	T* construct(const char* label = "?", const allocate_options& options = allocate_options()) { void* p = allocate(sizeof(T), label, options); return (T*)new (p) T(); }
	
	template<typename T>
	T* construct_array(size_t capacity, const char* label = "?", const allocate_options& options = allocate_options())
	{
		T* p = (T*)allocate(sizeof(T) * capacity, label, options);
		for (size_t i = 0; i < capacity; i++)
			new (p + i) T();
		return p;
	}

	template<typename T>
  void destroy(T* p) { if (p) { p->~T(); deallocate(p); } }

	template<typename T>
	void destroy_array(T* p, size_t capacity) { if (p) { for (size_t i = 0; i < capacity; i++) p[i].~T(); deallocate(p); } }

private:
	allocate_fn allocate_;
	deallocate_fn deallocate_;
};

// _____________________________________________________________________________
// Default implementations

extern allocator default_allocator;
extern allocator noop_allocator;

}
