// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Interface for accessing common debugger features

#pragma once

#include <oString/path.h>
#include <thread>

namespace ouro { namespace debugger {

enum class exception_code
{
	pure_virtual_call,
	unexpected,
	new_out_of_memory,
	invalid_parameter,
	debugger_exception,
	access_violation,
	illegal_instruction,
	stack_overflow,
	guard_page,
	array_bounds_exceeded,
	flt_denormal_operand,
	flt_divide_by_zero,
	flt_inexact_result,
	flt_invalid_operation,
	flt_overflow,
	flt_stack_check,
	flt_underflow,
	int_divide_by_zero,
	int_overflow,
	datatype_misalignment,
	priv_instruction,
	in_page_error,
	noncontinuable_exception,
	invalid_disposition,
	invalid_handle,
	breakpoint,
	single_step,
	sigabrt,
	sigint,
	sigterm,
};

typedef size_t symbol_t;

struct symbol_info
{
	symbol_t address;
	path_t module;
	path_t filename;
	mstring name;
	uint32_t symboloffset;
	uint32_t line;
	uint32_t charoffset;
};

// Sets the name of the specified thread in the debugger's UI. If the default id 
// value is specified then the id of this_thread is used.
void thread_name(const char* name, std::thread::id id = std::thread::id());

// Print the specified string to a debug window
void print(const char* string);

// Breaks in the debugger when the specified allocation occurs. The id can be
// determined from the output of a leak report.
void break_on_alloc(uintptr_t alloc_id);

// Fills out_symbols with up to num_symbols symbols of functions in the current 
// callstack from main(). This includes every function that led to the GetCallstack() 
// call. Offset ignores the first offset number of functions at the top of the 
// stack so a system can simplify/hide details of any debug reporting code and 
// keep the callstack started from the meaningful user code where an error or 
// assertion occurred. This returns the actual number of addresses retrieved.
size_t callstack(symbol_t* out_symbols, size_t num_symbols, size_t offset);
template<size_t size> inline size_t callstack(symbol_t (&out_symbols)[size], size_t offset)
{
#ifdef _DEBUG
	offset++; // ignore this inline function that isn't inlined in debug
#endif
	return callstack(out_symbols, size, offset);
}

// Convert a symbol retrieved using callstack() into more descriptive parts.
symbol_info translate(symbol_t symbol);

// uses snprintf to format the specified symbol to dst. If the bool
// pointed to by inout_is_std_bind is false, then this will print an "it's all 
// std::bind" line and update the value to true. If the bool contains true no
// string will be emitted and the return value will be zero. This way cluttering
// runs of std::bind code can be reduced to just one line.
int format(char* dst, size_t dst_size, symbol_t symbol, const char* prefix, bool* inout_is_std_bind = nullptr);
template<size_t size> int format(char (&dst)[size], symbol_t symbol, const char* prefix, bool* inout_is_std_bind = nullptr) { return format(dst, size, symbol, prefix, inout_is_std_bind); }
template<size_t capacity> int format(fixed_string<char, capacity>& dst, symbol_t symbol, const char* prefix, bool* inout_is_std_bind = nullptr) { return format(dst, dst.capacity(), symbol, prefix, inout_is_std_bind); }

// Uses snprintf to format the current callstack starting at offset to dst. If 
// filter_std_bind is true, std::bind internal functions will be skipped over for 
// a more compact callstack.
void print_callstack(char* dst, size_t dst_size, size_t offset, bool filter_std_bind);

// Saves a file containing debug information that can be used to do postmortem 
// debugging. Exceptions is the platform-specific context during exception
// handling.
bool dump(const path_t& path, bool full, void* exceptions);

// Optionally prompts the user, writes a dump file and terminates the process.
// Settings can be configured with win_exception_handler.
void dump_and_terminate(void* exception, const char* message);

}}
