// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>
#include <oCore/countof.h>
#include <oCore/timer.h>
#include <oMath/hlsl.h>
#include <oString/opttok.h>
#include <oString/string.h>
#include <array>
#include <chrono>

namespace ouro {

const char* as_string(const unit_test::result& r)
{
	switch (r)
	{
		case unit_test::result::success: return "success";
		case unit_test::result::failure: return "failure";
		case unit_test::result::notfound: return "not found";
		case unit_test::result::filtered: return "filtered";
		case unit_test::result::skipped: return "skipped";
		case unit_test::result::bugged: return "bugged";
		case unit_test::result::notready: return "not ready";
		case unit_test::result::leaks: return "leaks";
		case unit_test::result::perftest: return "perf test";
		default: break;
	}

	return "?";
}

namespace unit_test {

// unit tests must provide registered information
struct registered
{
	const char* name;      // unit test name
	test_fn run;           // function to run
	uint32_t flags;        // flags classifying the test
	uint32_t bug;          // if not zero, the unit test will be skipped citing this bug number
	const char* spawns[4]; // name processes this test spawns
	inline bool operator<(const registered& that) const { return _stricmp(name, that.name) < 0; }
};

// @tony this is not great, what can be done to make this better?
static std::array<registered, 200> s_registered_unit_tests;
uint32_t s_num_unit_tests;

register_to_framework::register_to_framework(const char* test_name, test_fn test, uint32_t flags, uint32_t bug)
{
	if (s_num_unit_tests >= s_registered_unit_tests.size())
		return;

	auto& r = s_registered_unit_tests[s_num_unit_tests++];

	r.name = test_name;
	r.run = test;
	r.flags = flags;
	r.bug = bug;
	r.spawns[0] = nullptr;
	r.spawns[1] = nullptr;
	r.spawns[2] = nullptr;
	r.spawns[3] = nullptr;
}

static print_type s_result_print_type[] = { print_type::success, print_type::fatal, print_type::error, print_type::caution, print_type::disabled, print_type::caution, print_type::disabled, print_type::caution, print_type::disabled, };
match_array_e(s_result_print_type, result);

static inline void separator(services& srv) { srv.printf(print_type::normal, "%c ", 179); }

static void apply_default_status(services& srv, const result& res, uint32_t test_bug)
{
	const char* msg = srv.status();

	switch (res)
	{
		case result::success:
			if (!*msg || !strcmp("operation was successful", msg))
				srv.status("---");
			break;

		case result::failure:
			if (!*msg)
				srv.status("Failed with no test-specific status message");
			break;

		case result::skipped:
			if (!*msg)
				srv.status("Skipped");
			break;

		case result::bugged:
			if (!*msg)
				srv.status("Test disabled. See oBug_%u", test_bug);
			break;

		case result::notready:
			if (!*msg)
				srv.status("Test not yet ready. See oBug_%u", test_bug);
			break;

		case result::leaks:
			if (!*msg)
				srv.status("Leaks (see debug log for full report)");
			break;

		case result::perftest:
			if (!*msg)
				srv.status("Potentially lengthy perf-test not run by default");
			break;

		default:
			break;
	}
}

static result run(framework_services& fw, const info_t& info, const registered* tests, size_t num_tests, const filter_chain::filter_t* filters, size_t num_filters)
{
	services& srv = fw;

	try { fw.check_system_requirements(info); }
	catch (std::exception& e)
	{
		throw std::runtime_error(std::string("unit tests did not meet system requirements:\n  ") + e.what());
	}

	filter_chain filter;
	try { filter = std::move(filter_chain(filters, num_filters)); }
	catch (std::exception& e)
	{
		srv.printf(print_type::error, "%s\n", e.what());
		return result::failure;
	}

	// sort tests
	std::vector<const registered*> sorted_tests;
	{
		sorted_tests.reserve(num_tests);
		auto t = tests;
		auto e = t + num_tests;
		while (t < e)
			sorted_tests.push_back(t++);
		std::sort(sorted_tests.begin(), sorted_tests.end(), [](const registered* a, const registered* b)->bool { return *a < *b; } );
	}

	// prepare formatting used to print results
	char fmtname  [64]; if (64 < snprintf(fmtname,   "%%-%us", info.name_col_width  )) oThrow(std::errc::invalid_argument, "name col width too wide");
	char fmtstatus[64]; if (64 < snprintf(fmtstatus, "%%-%us", info.status_col_width)) oThrow(std::errc::invalid_argument, "status col width too wide");
	char fmttime  [64]; if (64 < snprintf(fmttime,   "%%-%us", info.time_col_width  )) oThrow(std::errc::invalid_argument, "time col width too wide");
	char fmtmsg   [64];          snprintf(fmtmsg,    "%%s\n"                        );

	uint32_t nsucceeded = 0;
	uint32_t nfailed    = 0;
	uint32_t nleaks     = 0;
	uint32_t nskipped   = 0;

	std::array<uint32_t, (int)result::count> counts;

	fw.pre_iterations(info);

	timer whole_run_timer;
	for (size_t iteration = 0; iteration < info.num_iterations; iteration++)
	{
		counts.fill(0);
		srv.printf(print_type::normal, "========== %s Run %u ==========\n", info.test_suite_name, iteration + 1);

		// table headers
		{
			srv.printf(print_type::heading, fmtname,   "TEST NAME");       separator(srv);
			srv.printf(print_type::heading, fmtstatus, "STATUS");          separator(srv);
			srv.printf(print_type::heading, fmttime,   "TIME");            separator(srv);
			srv.printf(print_type::heading, fmtmsg,    "STATUS MESSAGE");
		}

		timer iter_timer;
		for (auto test : sorted_tests)
		{
			if (!test)
				continue;

			const char* test_name = test->name;

			srv.printf(print_type::normal, fmtname, test_name);
			separator(srv);
			
			result res = result::filtered;

			double test_run_time = 0.0;
			if (filter.passes(test_name))
			{
				srv.trace("========== Begin %s Run %u ==========", test_name, iteration + 1);
				
				// restore initial state
				fw.seed_rand(info.random_seed);
				
				// clear status
				fw.status("");

				fw.pre_test(info, test_name);

				timer test_timer;
				try
				{
					test->run(srv);
					res = result::success;
				}

				catch (skip_test& e)
				{
					res = result::skipped;
					srv.status(e.what());
				}

				catch (test_not_ready& e)
				{
					res = result::notready;
					srv.status(e.what());
				}

				catch (std::exception& e)
				{
					res = result::failure;
					srv.trace("%s: %s", test_name, e.what());
					srv.status(e.what());
				}

				double test_run_time = test_timer.seconds();

				fw.post_test(info);

				// check for leaks
				if (res != result::failure && fw.has_memory_leaks(info))
					res = result::leaks;

				char duration[64];
				format_duration(duration, round(test_run_time));
				srv.trace("========== End %s Run %u %s in %s ==========", test_name, iteration + 1, as_string(res), duration);
			}

			else
			{
				res = result::skipped;
				srv.status("---");
			}

			counts[(int)res]++;

			// print the result
			srv.printf(s_result_print_type[(int)res], fmtstatus, as_string(res));
			separator(srv);

			// print the time taken
			{
				double runtime = test_run_time;
				print_type type = runtime > info.run_time_very_slow ? print_type::error : ((runtime > info.run_time_slow) ? print_type::caution : print_type::normal);
				char duration[64];
				format_duration(duration, round(runtime), true);
				srv.printf(type, fmttime, duration);
				separator(srv);
			}

			// print the status message
			{
				apply_default_status(srv, res, test->bug);
				srv.printf(print_type::normal, fmtmsg, srv.status());
				::_flushall();
			}
		}

		::_flushall();

		// Summarize results for this iteration and sum total statistics
		const uint32_t iter_nsucceeded = counts[(int)result::success];
		const uint32_t iter_nfailed    = counts[(int)result::failure];
		const uint32_t iter_nleaks     = counts[(int)result::leaks]; 
		const uint32_t iter_nskipped   = counts[(int)result::skipped] + counts[(int)result::filtered] + counts[(int)result::bugged] + counts[(int)result::notready];

		if ((iter_nsucceeded + iter_nfailed + iter_nleaks) == 0)
		{
  		srv.printf(print_type::error, "========== Unit Tests: ERROR NO TESTS RUN ==========\n");
			break;
		}

    else
		{
			char duration[64];
			format_duration(duration, round(iter_timer.seconds()));
		  srv.printf(print_type::heading, "========== Unit Tests: %u succeeded, %u failed, %u leaked, %u skipped in %s ==========\n", iter_nsucceeded, iter_nfailed, iter_nleaks, iter_nskipped, duration);
		}

		nsucceeded += iter_nsucceeded;
		nfailed    += iter_nfailed;
		nleaks     += iter_nleaks;
		nskipped   += iter_nskipped;
	}
	
	if (info.num_iterations != 1)
	{
		char duration[64];
		format_duration(duration, round(whole_run_timer.seconds()));
		srv.printf(print_type::heading, "========== %u Iterations: %u succeeded, %u failed, %u leaked, %u skipped in %s ==========\n", info.num_iterations, nsucceeded, nfailed, nleaks, nskipped, duration);
	}

	if ((nsucceeded + nfailed + nleaks) == 0)
		return result::notfound;

  if (nfailed > 0)
    return result::failure;

	if (nleaks > 0)
		return result::leaks;

  return result::success;
}

static const option s_cmdline_opts[] = 
{
	{ 'i', "include",              "regex",         "regex matching a test name to include" },
	{ 'e', "exclude",              "regex",         "regex matching a test name to exclude" },
	{ 'n', "repeat-number",        "num iters",     "Repeat the tests this many times." },
	{ 'r', "random-seed",          "seed",          "Set the random seed to be used for this run. This is reset at the start of each test." },
	{ 'c', "capture-callstack",    nullptr,         "Capture full callstack to allocations for per-test leaks (slow!)" },
	{ '!', "break-on-alloc",       "alloc ordinal", "Break into debugger when the specified allocation occurs" },
	{ 't', "skip-terminate",       nullptr,         "Skips check of other processes to terminate" },
//{ 'p', "path",                 "path",          "Path where all test data is loaded from. The current working directory is used by default." },
//{ 's', "special-mode",         "mode",          "Run the test harness in a special mode (used mostly by multi-process/client-server unit tests)" },
//{ 'b', "golden-binaries",      "path",          "Path where all known-good \"golden\" binaries are stored. The current working directory is used by default." },
//{ 'g', "golden-images",        "path",          "Path where all known-good \"golden\" images are stored. The current working directory is used by default." },
//{ 'z', "output-golden-images", nullptr,         "Copy golden images of error images to the output as well, renamed to <image>_golden.png." },
//{ 'o', "output",               "path",          "Path where all logging and error images are created." },
//{ 'd', "disable-timeouts",     nullptr,         "Disable timeouts, mainly while debugging." },
//{ '_', "disable-leaktracking", nullptr,         "Disable the leak tracking when it is suspected of causing performance issues" },
//{ 'a', "automated",            nullptr,         "Run unit tests in automated mode, disable dialog boxes and exit on critical failure" },
//{ 'l', "logfile",              "path",          "Uses specified path for the log file" },
//{ 'x', "exhaustive",           nullptr,         "Run tests in exhaustive mode. Probably should only be run in Release. May take a very long time." },
};

char* print_command_line_docs(char* dst, size_t dst_size, const char* app_name)
{
	return optdoc(dst, dst_size, app_name, s_cmdline_opts);
}

size_t parse_command_line(int argc, const char* argv[], info_t* out_info, filter_chain::filter_t* out_filters, size_t max_filters)
{
	size_t nfilters = 0;

	info_t info;
	info.random_seed = timer::now_msi();

	const char* value = nullptr;
	char ch = opttok(&value, argc, argv, s_cmdline_opts);
	while (ch)
	{
		int ivalue = atoi(value);
		switch (ch)
		{
			case 'i':
			case 'e':
			{
				if (out_filters && nfilters < max_filters)
				{
					out_filters[nfilters].regex = value;
					out_filters[nfilters].type = ch == 'i' ? filter_chain::inclusion::include1 : filter_chain::inclusion::exclude1;
				}
				nfilters++;
				break;
			}

			case 'r': info.random_seed = atoi(value);         break;
			case 'n': info.num_iterations = atoi(value);      break;
			case '_': info.enable_leak_tracking = false;      break;
			case 'c': info.capture_leak_callstack = true;     break;
			case '!': info.break_on_alloc = ivalue;           break;
			case 't': info.terminate_other_processes = false; break;

			default: break;
		}

		ch = opttok(&value);
	}

	if (out_info)
		*out_info = info;
	
	return nfilters;
}

result run_tests(framework_services& fw, const info_t& info, const filter_chain::filter_t* filters, size_t num_filters)
{
	return run(fw, info, s_registered_unit_tests.data(), s_num_unit_tests, filters, num_filters);
}

}}
