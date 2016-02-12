// $(header)
#include "unit_test_framework.h"
#include <oSystem/debugger.h>
#include <oSystem/filesystem.h>
#include <oSystem/module.h>
#include <oSystem/reporting.h>
#include <oGUI/console.h>
#include <oGUI/msgbox.h>
#include <oGUI/msgbox_reporting.h>
#include <oGUI/Windows/win_gdi_bitmap.h>
#include "resource.h"

#include <oSystem/windows/win_exception_handler.h>

using namespace ouro;

static const char* sTITLE = "Ouroboros Unit Test Suite";

static void init_reporting()
{
	reporting::set_prompter(prompt_msgbox);
	reporting::info ri;
	ri.prefix_thread_id = false;
	reporting::set_info(ri);
}

static void init_console()
{
	module::info mi = this_module::get_info();
	char ver[16];
	char title[128];
	snprintf(title, "%s v%s%s", sTITLE, to_string(ver, mi.version), mi.is_special ? "*" : "");
	console::set_title(title);

	// resize console
	try
	{
		console::info_t i;
		i.buffer_size = int2(255, 1024);
		i.window_position = int2(10, 10);
		i.window_size = int2(120, 50);
		i.foreground = console_color::light_green;
		i.background = console_color::black;
		console::set_info(i);
	}
	catch (std::exception& e)
	{
		e;
		oTRACEA("console::set_info failed: %s", e.what());
	}

#if defined(WIN64) || defined(WIN32)
	// set icon
	{
		windows::gdi::scoped_icon icon = windows::gdi::load_icon(IDI_APPICON);
		console::icon((icon_handle)(HICON)icon);
	}
#endif
}

static void delete_stale_logs(const char* exe_suffix = nullptr)
{
	// delete any more than this count
	const size_t kLogHistory = 10;

	path_t log_file_wildcard = filesystem::log_path(true, exe_suffix);
	log_file_wildcard.replace_extension_with_suffix("_*.stdout");

	struct file_desc
	{
		path_t path;
		time_t last_written;

		// sort newer to older
		bool operator<(const file_desc& that)
		{
			return last_written > that.last_written;
		}
	};

	std::vector<file_desc> logs;
	logs.reserve(20);

	filesystem::enumerate(log_file_wildcard, [](const path_t& path, const filesystem::file_status& status, uint64_t size, void* user)->bool
	{
		auto& logs = *(std::vector<file_desc>*)user;
		file_desc fd;
		fd.path = path;
		fd.last_written = filesystem::last_write_time(path);
		logs.push_back(fd);
		return true;
	}, &logs);

	if (logs.size() > kLogHistory)
	{
		std::sort(logs.begin(), logs.end());
		logs.resize(kLogHistory);
		for (auto l : logs)
		{
			filesystem::remove_filename(l.path);
			filesystem::remove_filename(l.path.replace_extension(".stderr"));
		}
	}
}

static void init_log(const char* exe_suffix = nullptr)
{
	path_t log_path = filesystem::log_path(true, exe_suffix);
	auto log_ext = log_path.extension();
	
	// Log stdout (that which is printed to the console TTY)
	{
		path_t log_stdout(log_path);
		log_stdout.replace_extension(".stdout");
		log_stdout.append(log_ext, false);
		console::set_log(log_stdout);
	}

	// Log stderr (that which is printed to the debug TTY)
	{
		path_t log_stderr(log_path);
		log_stderr.replace_extension(".stderr");
		log_stderr.append(log_ext, false);
		reporting::set_log(log_stderr);
	}

	// Create a place for the dump file
	{
		path_t dump_path = filesystem::app_path(true);
		dump_path.replace_extension();
		if (exe_suffix)
		{
			sstring suffix;
			snprintf(suffix, "-%s", exe_suffix);
			dump_path /= suffix;
		}

		windows::exception::mini_dump_path(dump_path);
		windows::exception::prompt_after_dump(false);
	}
}

static bool find_dup_proc_by_name(process::id pid, process::id parent_pid, const char* exe_path, process::id ignore_pid, const char* find_name, process::id* out_pid)
{
	if (ignore_pid != pid && !_stricmp(find_name, exe_path))
	{
		*out_pid = pid;
		return false;
	}

	return true;
}

static bool terminate_duplicate_instances(const char* name, bool prompt)
{
	process::id this_pid = this_process::get_id();
	process::id dup_pid;

	auto en = std::bind(find_dup_proc_by_name
		, std::placeholders::_1
		, std::placeholders::_2
		, std::placeholders::_3
		, this_pid
		, name
		, &dup_pid);

	process::enumerate(en);

	while (dup_pid)
	{
		msg_result result = msg_result::yes;
		if (prompt)
			result = msgbox(msg_type::yesno, nullptr, sTITLE, "An instance of the unittest executable was found at process %u. Do you want to kill it now? (no means this application will exit)", dup_pid);

		if (result == msg_result::no)
			return false;

		process::terminate(dup_pid, std::errc::operation_canceled);
		if (!process::wait_for(dup_pid, std::chrono::seconds(5)))
			msgbox(msg_type::yesno, nullptr, sTITLE, "Cannot terminate stale process %u, please end this process before continuing.", dup_pid);

		dup_pid = process::id();
		process::enumerate(en);
	}

	return true;
}

static bool ensure_one_instance_running(bool prompt)
{
#ifdef _DEBUG
	static const char* kModuleDebugSuffix = "D";
#else
	static const char* kModuleDebugSuffix = "";
#endif

	// Scan for both release and debug builds already running
	path_t tmp = filesystem::app_path(true);
	path_t path(tmp);
	path.remove_basename_suffix(kModuleDebugSuffix);
	path_t relname = path.filename();

	path_t dbgname(relname);
	dbgname.insert_basename_suffix(kModuleDebugSuffix);

	if (!terminate_duplicate_instances(dbgname, prompt))
		return false;
	
	if (!terminate_duplicate_instances(relname, prompt))
		return false;

	return true;
}

int main(int argc, const char* argv[])
{
	init_reporting();
	init_console();

	std::vector<filter_chain::filter_t> filters;
	filters.resize(32);

	unit_test::info_t info;
	size_t nfilters = 32;
	
	do
	{
		filters.resize(nfilters);
		nfilters = unit_test::parse_command_line(argc, argv, &info, filters.data(), filters.size());

	} while (nfilters > filters.size());

	filters.resize(nfilters);

	// get this hooked asap
	if (info.break_on_alloc)
		debugger::break_on_alloc(info.break_on_alloc);

	if (!info.enable_dialogs)
		windows::exception::enable_dialogs(false);

	// skip unmodified codes' tests
	auto_filter_libs(filters);

	delete_stale_logs();
	init_log();

	ouro_unit_test_framework framework;

	unit_test::result result = unit_test::result::success;
	
	if (ensure_one_instance_running(true))
		result = unit_test::run_tests(framework, info, filters.data(), filters.size());
	else
		result = unit_test::result::skipped;

	if (this_process::has_debugger_attached())
	{
		::system("echo.");
		::system("pause");
	}

	oTRACEA("Unit test exiting with result: %s", as_string(result));

	::_flushall();
	return (int)result;
}
