// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// unit test framework

#pragma once
#include <oCore/assert.h>
#include <oCore/blob.h>
#include <oCore/filter_chain.h> // unit_test needs to operate without this. Only needed by infrastructure, not in-each-unit-test header.
#include <cstdarg>
#include <cstdint>
#include <stdexcept>
#include <vector>

// _____________________________________________________________________________
// Test declaration

/* Define a unit test in the following manner:

oTEST(my_test)
{
	srv.status("all good");
}

*/

// Flags used to describe tests (not much here yet)
#define oTEST_NORMAL 0

#define oTEST2(test_name, test_flags, bug) \
	namespace ouro { namespace unit_test { void test_##test_name(ouro::unit_test::services&); }} \
	namespace { ouro::unit_test::register_to_framework register_test_##test_name(#test_name, ouro::unit_test::test_##test_name, test_flags, bug); } \
	void ouro::unit_test::test_##test_name(ouro::unit_test::services& srv)

#define oTEST(test_name) oTEST2(test_name, oTEST_NORMAL, 0)

// _____________________________________________________________________________
// Error handling and checking

// Use these to simplify test, but any exception can be thrown on error in a test
#define oCHECK(expr, format, ...) oCheck(expr, std::errc::invalid_argument, format, ## __VA_ARGS__)
#define oCHECK0(expr) oCheck(expr, std::errc::invalid_argument, "")

namespace ouro { namespace surface { class image; } namespace unit_test {

// throw this from tests that determine they want to be skipped
class skip_test : public std::runtime_error
{
public:
	skip_test() : std::runtime_error("") {}
	skip_test(const std::string& what) : std::runtime_error(what) {}
};

// throw this from tests that aren't ready to be included yet
class test_not_ready : public std::runtime_error
{
public:
	test_not_ready() : std::runtime_error("") {}
	test_not_ready(const std::string& what) : std::runtime_error(what) {}
};

// _____________________________________________________________________________
// Configuration, framework & services

enum class print_type
{
	normal,
	info,
	heading,
	disabled,
	success,
	caution,
	error,
	fatal,

	count,
};

enum class result
{
	success,   // all passes
	failure,   // something failed
	notfound,  // test not found
	filtered,  // skipped by command line filter
	skipped,   // skipped by test itself
	bugged,    // registered with a bug so was not run
	notready,  // in development, not quite ready
	leaks,     // succeeded but had memory leaks on exit
	perftest,  // test is designed to measure performance, not correctness (often takes longer, uses more system resources)

	count,
};

struct info_t
{
	info_t()
		: test_suite_name("")
		, num_iterations(1)
		, random_seed(0)
		, async_file_timeout_ms(20000)
		, diff_image_mult(8)
		, name_col_width(40)
		, time_col_width(5)
		, status_col_width(9)
		, break_on_alloc(0)
		, run_time_slow(10.0f)
		, run_time_very_slow(20.0f)
		, max_rms_error(1.0f)
		, enable_dialogs(true)
		, enable_leak_tracking(true)
		, capture_leak_callstack(false)
		, terminate_other_processes(true)
	{}

	const char* test_suite_name;
	uint32_t num_iterations;
	uint32_t random_seed;
	uint32_t async_file_timeout_ms;
	uint32_t diff_image_mult;
	uint32_t name_col_width;
	uint32_t time_col_width;
	uint32_t status_col_width;
	uint32_t break_on_alloc;
	float run_time_slow;
	float run_time_very_slow;
	float max_rms_error;
	bool enable_dialogs;
	bool enable_leak_tracking;
	bool capture_leak_callstack; // slow! filter tests carefully
	bool terminate_other_processes;
};

// abstraction for platform implementations required by unit tests and the framework
class services
{
public:
	
	// write to tty
	virtual void vtrace(const char* format, va_list args) = 0;
	inline  void  trace(const char* format, ...) { va_list args; va_start(args, format); vtrace(format, args); va_end(args); }

	// write to stdout
	virtual void vprintf(const print_type& type, const char* format, va_list args) = 0;
	inline  void  printf(const print_type& type, const char* format, ...) { va_list args; va_start(args, format); vprintf(type, format, args); va_end(args); }

	// write to an internal buffer to be displayed in the status column
	virtual void vstatus(const char* format, va_list args) = 0;
	inline  void  status(const char* format, ...) { va_list args; va_start(args, format); vstatus(format, args); va_end(args); }

	// returns string of last call to report or vreport
	virtual const char* status() const = 0;

	// all data loaded should be relative to this path
	virtual const char* root_path() const = 0;

	// returns a random number the see
	virtual int rand() = 0;

	virtual bool is_debugger_attached() const = 0;

	virtual bool is_remote_session() const = 0;

	virtual size_t total_physical_memory() const = 0;

	// fills in percent usage of the CPU by this process
	virtual void get_cpu_utilization(float* out_avg, float* out_peek) = 0;

	// resets the frame of recording CPU utilization
	virtual void reset_cpu_utilization() = 0;

	// Load the entire contents of the specified file into a newly allocated 
	// buffer.
	virtual blob load_buffer(const char* path) = 0;

	// This function compares the specified surface to a golden image named after
	// the test's name suffixed with nth. If nth is 0 then the golden 
	// image should not have a suffix. If max_rms_error is negative a default 
	// should be used. If the surfaces are not similar this throws an exception.
	virtual void check(const surface::image& img, int nth = 0, float max_rms_error = -1.0f) = 0;
};

// abstraction for platform implementations required by the framework only
class framework_services : public services
{
public:
	// run at start, this should throw if requirements for memory, cpu, 
	// driver, etc. aren't met
	virtual void check_system_requirements(const info_t& info) = 0;

	// run once before the iteration loop
	virtual void pre_iterations(const info_t& info) = 0;

	// rand seed is restored before each test is run using this api
	virtual void seed_rand(uint32_t seed) = 0;

	// run before/after each test
	virtual void pre_test(const info_t& info, const char* test_name) = 0;
	virtual void post_test(const info_t& info) = 0;

	virtual bool has_memory_leaks(const info_t& info) = 0;
};

// all unit tests are functions of this type
// throw execeptions on error
// use services::status() to output a final message
typedef void (*test_fn)(services& srv);

class register_to_framework
{
public:
	register_to_framework(const char* test_name, test_fn test, uint32_t flags, uint32_t bug);
};

// prints the docs to the specified buffer (need a few KB)
char* print_command_line_docs(char* dst, size_t dst_size, const char* app_name);

// parses a typical command-line for a unit test application as well as populates an array of filters and returns the count
// count can be more than max_filters, hinting that a bigger buffer should've been specified, but appending will stop at max_filters
size_t parse_command_line(int argc, const char* argv[], info_t* out_info, filter_chain::filter_t* out_filters, size_t max_filters);

// runs all tests according to filters with info's settings
result run_tests(framework_services& fw, const info_t& info, const filter_chain::filter_t* filters, size_t num_filters);

}}
