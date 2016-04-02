// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Interface for maintaining details of an allocator, mainly for leak tracking.

#pragma once
#include <oConcurrency/concurrent_hash_map.h>
#include <oConcurrency/countdown_latch.h>
#include <oMemory/allocate.h>
#include <oMemory/concurrent_object_pool.h>
#include <atomic>
#include <cstdint>

namespace ouro {

class leak_tracker
{
public:
	typedef uint32_t size_type;
	typedef size_t symbol_type;

	static const size_type stack_trace_max_depth = 32; // how many entries to save
	static const size_type stack_trace_offset = 8; // start at nth entry (bypass common infrastructure code)
	static const size_type std_bind_internal_offset = 5; // number of symbols internal to std::bind to skip

	// Tracking can come from 3rd party libraries with their own idea of timing
	// when freeing memory. For example TBB keeps its threads around in a way that 
	// cannot be determined from the outside, so provide a per-thread bool value
	// to allow per-thread enabling/disabling for such allocations. This is not 
	// just a thread_local value because DLLs can have their own copies and thus
	// create not only racy leaks, but also ones that only occur when called in a
	// certain order from different DLLs.
	typedef bool& (*thread_local_tracking_enabled_fn)();

	// Returns the number of symbols retreived into _pSymbols. _Offset allows
	// the capture to ignore internal details of the capture.
	typedef size_t (*callstack_fn)(symbol_type* symbols, size_t num_symbols, size_t offset);

	// snprintf to the specified destination the decoded symbol_t with optional
	// prefix. This should also be able to detect if the symbol_t is a std::bind
	// detail and set the optional bool accordingly, optionally skipping the 
	// noisy inner details of std::bind.
	typedef int (*format_fn)(char* dst, size_t dst_size, symbol_type symbol
		, const char* prefix, bool* out_is_std_bind);

	// Print a fixed string to some destination
	typedef void (*print_fn)(const char* str);

	struct init_t
	{
		init_t()
			: thread_local_tracking_enabled(nullptr)
			, callstack(nullptr)
			, format(nullptr)
			, print(nullptr)
			, expected_delay_ms(5000)
			, unexpected_delay_ms(2000)
			, use_hex_for_alloc_id(false)
			, capture_callstack(false)
		{}

		thread_local_tracking_enabled_fn thread_local_tracking_enabled;
		callstack_fn callstack;
		format_fn format;
		print_fn print;

		uint32_t expected_delay_ms;
		uint32_t unexpected_delay_ms;
		bool use_hex_for_alloc_id;
		bool capture_callstack;
	};

	// returns the number of bytes to record the specified number of allocs
	static size_type calc_size(size_type capacity);

	
	// non-concurrent api
	
	leak_tracker();
	leak_tracker(const init_t& i, size_type capacity, const char* alloc_label = "leak_tracker", const allocator& a = default_allocator);
	leak_tracker(const init_t& i, void* memory, size_type capacity);
	leak_tracker(leak_tracker&& that);
	~leak_tracker();
	leak_tracker& operator=(leak_tracker&& that);

	// initializes the queue with memory allocated from allocator
	void initialize(const init_t& i, size_type capacity, const char* alloc_label = "leak_tracker", const allocator& a = default_allocator);

	// use calc_size() to determine memory size
	void initialize(const init_t& i, void* memory, size_type capacity);

	// deinitializes the hash map and returns the memory passed to initialize()
	void* deinitialize();

	// ?
	void thread_local_tracking(bool enabled) { init.thread_local_tracking_enabled() = enabled; }
	bool thread_local_tracking() const { return init.thread_local_tracking_enabled(); }

	// Slow! but will pinpoint exactly where a leak was allocated.
	void capture_callstack(bool enabled) { init.capture_callstack = enabled; }
	bool capture_callstack() const { return init.capture_callstack; }

	// Some operations are asynchronous and allocate memory. If such operations 
	// occur near the end of the application's life, then there is an opportunity
	// to delay the report a bit waiting for the operation to complete. A memory 
	// user in such a case can call add_delay() and when its memory is freed then 
	// release_delay(). This only delays reporting - because the thread allocating 
	// memory could be terminated, this will only block for a time - not forever.
	void add_delay() { delay_latch.reference(); }
	void release_delay() { delay_latch.release(); }

	// Clears all allocation tracking.
	void reset() { allocs.clear(); }

	// Reports all allocations currently tracked to the debugger print function.
	// If current_context_only is true, then only the allocations since the last 
	// call to new_context() will be reported. If current_context_only is false, 
	// all allocations since the start of tracking will be reported. This returns 
	// the number of leaks reported.
	size_t report(bool current_context_only = true);


	// concurrent api

	// Only allocations within a context are reported as leaks. This is to get 
	// around bootstrap allocations for localized reporting. For example in a unit 
	// test infrastructure, this allows only leak detection for each specific test.
	void new_context() { current_context++; }

	// call this inside malloc/free calls to track by pointer
	void on_stat(const allocation_stats& stats, void* old_ptr = nullptr);

	// call this inside malloc/free calls to track by ordinal (MS debug CRT hits
	// callback before it gives a pointer, so all we have is the allocation id)
	void on_stat_ordinal(const allocation_stats& stats, uint32_t old_ordinal = uint32_t(-1));

private:
	struct entry
	{
		const char* label;
		size_t size;
		symbol_type stack[stack_trace_max_depth];
		uint8_t num_stack_entries;
		bool tracked; // true if the allocation occurred when tracking wasn't enabled
		uint16_t context;
		uint32_t id;
		bool operator==(const entry& that) { return id == that.id; }
		bool operator<(const entry& that) { return id < that.id; }
	};

	static const uint32_t nullidx = uint32_t(-1);

	typedef concurrent_hash_map<uint64_t, uint32_t> hash_map_t;
	typedef concurrent_object_pool<entry> pool_t;

	hash_map_t allocs;
	pool_t pool;
	countdown_latch delay_latch;
	std::atomic<uint16_t> current_context;
	init_t init;

	void internal_on_stat(uintptr_t new_ptr, const allocation_stats& stats, uintptr_t old_ptr = 0);
	size_t num_outstanding_allocations(bool current_context_only);
};

}
