// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "unit_test_framework.h"
#include <oCore/countof.h>
#include <oCore/color.h>
#include <oBase/scc.h>
#include <oConcurrency/concurrency.h>
#include <oSystem/adapter.h>
#include <oSystem/cpu.h>
#include <oSystem/filesystem.h>
#include <oSystem/process.h>
#include <oSystem/system.h>
#include <oSystem/windows/win_iocp.h>
#include <oSystem/windows/win_crt_leak_tracker.h>
#include <oGUI/console.h>

using namespace ouro;

static void trace_cpu_features(unit_test::services& srv)
{
	cpu::info_t cpu_info = cpu::get_info();
	srv.trace("CPU: %s. %d Processors, %d HWThreads", cpu_info.brand_string
		, cpu_info.processor_count, cpu_info.hardware_thread_count);

	cpu::enumerate_features([](const char* feature, const cpu::support& support, void* user)->bool
	{
		auto& srv = *(unit_test::services*)user;
		srv.trace("- %s: %s", feature, as_string(support));
		return true;
	}, &srv);
}

static adapter::info_t find_adapter()
{
	adapter::info_t info;

	adapter::enumerate([](const adapter::info_t& info, void* user)->bool
	{
		auto found = (adapter::info_t*)user;
		if (info.version >= adapter::minimum_version(info.vendor))
		{
			*found = info;
			return false;
		}
		return true;
	}, &info);

	return info;
}

static void print_header(const unit_test::info_t& info, unit_test::services& srv)
{
	trace_cpu_features(srv);

	oTRACEA("Aero is %sactive", system::uses_gpu_compositing() ? "" : "in");
	oTRACEA("Remote desktop is %sactive", system::is_remote_session() ? "" : "in");

	srv.printf(unit_test::print_type::info, "Root Path: %s\n", srv.root_path());
	srv.printf(unit_test::print_type::info, "Random Seed: %u\n", info.random_seed);
		
	auto adapter_info = find_adapter();
	if (adapter_info.vendor == vendor::unknown)
		srv.printf(unit_test::print_type::info, "Video Driver: no compatible driver found\n");
	else
	{
		char buf[32];
		srv.printf(unit_test::print_type::info, "Video Driver: %s v%s\n", adapter_info.description.c_str(), to_string(buf, adapter_info.version));
	}

	auto scc = make_scc(scc_protocol::svn, 
	[](const char* cmdline, scc_get_line_fn get_line, void* user, uint32_t timeout_ms)->int
	{
		return system::spawn_for(cmdline, get_line, user, false, timeout_ms);

	}, nullptr);

	path_t dev = filesystem::dev_path();
	lstring CLStr;
	uint CL = 0;//scc->revision(dev); //@tony  this takes too long
	if (CL)
	{
		try
		{
			if (scc->is_up_to_date(dev, CL))
				snprintf(CLStr, "%d", CL);
			else
				snprintf(CLStr, "%d + modifications", CL);
		}

		catch (std::exception& e)
		{
			snprintf(CLStr, "%d + ???", CL);
			srv.trace("scc failure: %s", e.what());
		}
	}
	else
		snprintf(CLStr, "???");

	srv.printf(unit_test::print_type::info, "SCC Revision: %s\n", CLStr.c_str());
}

void ouro_unit_test_framework::check_system_requirements(const unit_test::info_t& info)
{
	unit_test_info_ = info;

	// Situation:
	// exe statically linked to oooii lib
	// dll statically linked to oooii lib
	// exe hard-linked to dll
	//
	// In this case we're seeing that it is possible for DllMain 
	// to be called on a thread that is NOT the main thread. Strange, no?
	// This would cause TBB, the underlying implementation of ouro::parallel_for
	// and friends, to be initialized in a non-main thread. This upsets TBB,
	// so disable static init of TBB and force initialization here in a 
	// function known to execute on the main thread.
	//
	// @tony: TODO: FIND OUT - why can DllMain execute in a not-main thread?

	ensure_scheduler_initialized();

	bool up_to_date = true;
	adapter::info_t adapter_info;
	adapter::enumerate([](const adapter::info_t& adapter_info, void* user)->bool
	{
		auto& out_adapter_info = *(adapter::info_t*)user;
		if (adapter_info.version >= adapter::minimum_version(adapter_info.vendor))
		{
			out_adapter_info = adapter_info;
			return false;
		}
		return true;
	}, &adapter_info);

	if (adapter_info.vendor == vendor::unknown)
		throw std::exception("no suitable video driver found");

	root_ = filesystem::data_path();

	path_t golden_images_path = root_ / "GoldenImages";
	path_t failed_images_path = root_ / "FailedImageCompares";

	// initialize image-based unit testing
	{
		golden_image::init_t i;
		i.adapter_id = adapter_info.id;
		i.golden_path_root = golden_images_path;
		i.failed_path_root = failed_images_path;
		golden_image_.initialize(i);
	}
}

void ouro_unit_test_framework::pre_iterations(const unit_test::info_t& info)
{
	print_header(info, *this);

	// initialize iocp before leak tracking flags it as a leak
	windows::iocp::ensure_initialized();

	// enable leak tracking according to options
	windows::crt_leak_tracker::enable(info.enable_leak_tracking);
	windows::crt_leak_tracker::capture_callstack(info.capture_leak_callstack);
}

void ouro_unit_test_framework::seed_rand(uint32_t seed)
{
	srand(seed);
}

void ouro_unit_test_framework::pre_test(const unit_test::info_t& info, const char* test_name)
{
	test_name_ = test_name;

	//filesystem::remove(path_t(??TempPath??));

	// before each test reset leak tracking so one doesn't pollute the next
	windows::crt_leak_tracker::new_context();
}
	
void ouro_unit_test_framework::post_test(const unit_test::info_t& info)
{
	// wait for any async i/o to finish
	if (!windows::iocp::wait_for(info.async_file_timeout_ms))
		trace("iocp wait timed out");
}

bool ouro_unit_test_framework::has_memory_leaks(const unit_test::info_t& info)
{
	return windows::crt_leak_tracker::report();
}

void ouro_unit_test_framework::vtrace(const char* format, va_list args)
{
	char buf[2048];
	ouro::vsnprintf(buf, format, args);
	oTRACEA("%s", buf);
}

void ouro_unit_test_framework::vprintf(const unit_test::print_type& type, const char* format, va_list args)
{
	static uint8_t s_type_fg[] = { console_color::purple, console_color::gray,  console_color::light_purple, console_color::gray , console_color::light_green, console_color::light_yellow, console_color::light_red,   console_color::light_yellow, };
	static uint8_t s_type_bg[] = { console_color::black,  console_color::black, console_color::black,        console_color::black, console_color::black,       console_color::black,        console_color::black,       console_color::light_red,    };
	match_array_e(s_type_fg, unit_test::print_type);
	match_array_e(s_type_bg, unit_test::print_type);

	console::vfprintf(stdout, s_type_fg[(int)type], s_type_bg[(int)type], format, args);
}

void ouro_unit_test_framework::vstatus(const char* format, va_list args)
{
	ouro::vsnprintf(status_, format, args);
}

const char* ouro_unit_test_framework::status() const
{
	return status_;
}

const char* ouro_unit_test_framework::root_path() const
{
	return root_.c_str();
}

int ouro_unit_test_framework::rand()
{
	return ::rand();
}

bool ouro_unit_test_framework::is_debugger_attached() const
{
	return this_process::has_debugger_attached();
}

bool ouro_unit_test_framework::is_remote_session() const
{
	return system::is_remote_session();
}

size_t ouro_unit_test_framework::total_physical_memory() const
{
	system::heap_info hi = system::get_heap_info();
	return static_cast<size_t>(hi.total_physical);
}

void ouro_unit_test_framework::get_cpu_utilization(float* out_avg, float* out_peek)
{
	ouro::process_stats_monitor::info stats = psm_.get_info();
	*out_avg = stats.average_usage;
	*out_peek = stats.high_usage;
}

void ouro_unit_test_framework::reset_cpu_utilization()
{
	psm_.reset();
}

ouro::blob ouro_unit_test_framework::load_buffer(const char* path)
{
	path_t p(path);
	if (!filesystem::exists(p))
		p = filesystem::data_path() / p;
	return filesystem::load(p);
}

void ouro_unit_test_framework::check(const ouro::surface::image& img, int nth, float max_rms_error)
{
	max_rms_error = max_rms_error >= 0.0f ? max_rms_error : unit_test_info_.max_rms_error;
	golden_image_.test(test_name_, img, nth, max_rms_error, unit_test_info_.diff_image_mult);
}

static bool scc_push_modified(std::vector<scc_file>& modified)
{
	modified.clear();
	modified.reserve(128);

	auto scc = make_scc(scc_protocol::svn, 
	[](const char* cmdline, scc_get_line_fn get_line, void* user, uint32_t timeout_ms)->int
	{
		return system::spawn_for(cmdline, get_line, user, false, timeout_ms);

	}, nullptr);

	path_t branch_path = filesystem::dev_path();

	try
	{ 
		scc->status(branch_path, 0, scc_visit_option::modified_only, [](const scc_file& file, void* user)
		{
			auto& modified = *(std::vector<scc_file>*)user;

			modified.push_back(file);
		}, &modified);
	}

	catch (std::system_error& e)
	{
		if (e.code() == std::errc::no_such_file_or_directory)
		{
			oTRACEA("'%s' must be in the path for filtering to work", as_string(scc->protocol()));
			return false;
		}
		else
			oTRACEA("scc could not find modified files. This may indicate '%s' is not accessible. (%s)", as_string(scc->protocol()), e.what());

		return false;
	}

	catch (std::exception& e)
	{
		e;
		oTRACEA("scc could not find modified files. This may indicate '%s' is not accessible. (%s)", as_string(scc->protocol()), e.what());
		return false;
	}

	return true;
}

static void scc_scan_for_changes(const char** paths, bool* has_changes, size_t num_paths)
{
	std::vector<scc_file> modified;
	if (!scc_push_modified(modified))
	{
		// cannot verify state, so assume its all dirty
		memset(has_changes, true, sizeof(bool) * num_paths);
		return;
	}

	// assume no changes
	memset(has_changes, false, sizeof(bool) * num_paths);

	// scan scc modified and if there mark as having changes
	char path[512];
	for (size_t i = 0; i < num_paths; i++)
	{
		snprintf(path, "/%s/", paths[i]);
		for (const auto& f : modified)
		{
			if (strstr(f.path, path))
			{
				has_changes[i] = true;
				break;
			}
		}
	}

	// scan again for derivative libs and mark them as having 
	// changes because their dependency has a change
	for (size_t i = 0; i < num_paths; i++)
	{
		bool this_has_changes = has_changes[i];
		if (!this_has_changes)
		{
			for (size_t j = 0; j < i; j++)
				if (has_changes[j])
					this_has_changes = true;
		}
	}
}

void auto_filter_libs(std::vector<ouro::filter_chain::filter_t>& filters)
{
	// @tony: Disabled in the OpenSource distro because running the unit 
	// tests is the only proof of life in the Ouroboros branch.
	// @tony: Not sure what this means anymore... the idea is that it's not
	// a meaningful first-experience for someone to download Ouroboros and 
	// have the unit tests noop across the board. To use this day in and day
	// out the auto-filter is helpful though. So what to set it for?
	static bool is_distro = /*true*/false;

	// Many libs are pretty stable these days, only test if there are changes.
	if (!filters.empty() || is_distro)
		return;

	static const char* s_libs[] =
	{
		"oArch",
		"oMemory",
		"oString",
		"oMath",
		"oConcurrency",
		"oBase",
		"oSurface",
		"oPrim",
		"oManip",
		"oCompute",
		"oMesh",
		"oGPU",
		"oCore",
		"oGfx",
		"oGUI",
	};

	static const char* s_patterns[] =
	{
		"oArch.*",
		"oMemory.*",
		"oString.*",
		"oMath.*",
		"oConcurrency.*",
		"oBase.*",
		"oSurface.*",
		"oPrim.*",
		"oManip.*",
		"oCompute.*",
		"oMesh.*",
		"oGPU.*",
		"oCore.*",
		"oGfx.*",
		"oGUI.*",
	};
	static_assert(countof(s_libs) == countof(s_patterns), "array mismatch");

	bool has_changes[countof(s_libs)];
	scc_scan_for_changes(s_libs, has_changes, countof(s_libs));

	// append an exclusion filter for anything that hasn't changed
	for (size_t i = 0; i < countof(has_changes); i++)
	{
		if (has_changes[i])
			continue;
		
		filter_chain::filter_t f;
		f.regex = s_patterns[i];
		f.type = filter_chain::inclusion::exclude1;
		filters.push_back(f);
	}
}

