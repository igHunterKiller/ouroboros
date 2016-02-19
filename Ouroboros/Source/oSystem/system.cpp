// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/finally.h>
#include <oCore/macros.h>
#include <oSystem/system.h>
#include <oSystem/process.h>
#include <oSystem/windows/win_util.h>
#include <oSystem/windows/win_version.h>
#include <oBase/date.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Dwmapi.h>
#include <Lm.h>
#include <PowrProf.h>
#include <Shellapi.h>

namespace ouro { namespace system {

heap_info get_heap_info()
{
	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(MEMORYSTATUSEX);
	oVB(GlobalMemoryStatusEx(&ms));
	heap_info hi;
	hi.total_used = ms.dwMemoryLoad;
	hi.avail_physical = ms.ullAvailPhys;
	hi.total_physical = ms.ullTotalPhys;
	hi.avail_virtual_process = ms.ullAvailVirtual;
	hi.total_virtual_process = ms.ullTotalVirtual;
	hi.avail_paged = ms.ullAvailPageFile;
	hi.total_paged = ms.ullTotalPageFile;
	return hi;
}

static void now(FILETIME* out_filetime, bool is_utc)
{
	if (is_utc)
		GetSystemTimeAsFileTime(out_filetime);
	else
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		SystemTimeToFileTime(&st, out_filetime);
	}
}

void now(ntp_timestamp* out_ntp_timestamp)
{
	FILETIME ft;
	now(&ft, true);
	*out_ntp_timestamp = date_cast<ntp_timestamp>(ft);
}

void now(ntp_date* out_ntp_date)
{
	FILETIME ft;
	now(&ft, true);
	*out_ntp_date = date_cast<ntp_date>(ft);
}

void now(date* out_date)
{
	FILETIME ft;
	now(&ft, true);
	*out_date = date_cast<date>(ft);
}

static date to_date(const SYSTEMTIME& system_time)
{
	date d;
	d.year = system_time.wYear;
	d.month = (month)system_time.wMonth;
	d.day = system_time.wDay;
	d.day_of_week = (weekday)system_time.wDayOfWeek;
	d.hour = system_time.wHour;
	d.minute = system_time.wMinute;
	d.second = system_time.wSecond;
	d.millisecond = system_time.wMilliseconds;
	return d;
}

static void from_date(const date& _date, SYSTEMTIME* out_system_time)
{
	out_system_time->wYear = (WORD)_date.year;
	out_system_time->wMonth = (WORD)_date.month;
	out_system_time->wDay = (WORD)_date.day;
	out_system_time->wHour = (WORD)_date.hour;
	out_system_time->wMinute = (WORD)_date.minute;
	out_system_time->wSecond = (WORD)_date.second;
	out_system_time->wMilliseconds = (WORD)_date.millisecond;
}

date from_local(const date& local)
{
	SYSTEMTIME in, out;
	from_date(local, &in);
	oVB(!SystemTimeToTzSpecificLocalTime(nullptr, &in, &out));
	return to_date(out);
}

date to_local(const date& utc)
{
	SYSTEMTIME in, out;
	from_date(utc, &in);
	oVB(!SystemTimeToTzSpecificLocalTime(nullptr, &in, &out));
	return to_date(out);
}

void reboot()
{
	set_privilege(privilege::may_shutdown);
	oVB(ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_FLAG_PLANNED));
}

void shutdown()
{
	set_privilege(privilege::may_shutdown);
	oVB(ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_FLAG_PLANNED));
}

void kill_file_browser()
{
	if (process::exists("explorer.exe"))
	{
		oTRACE("Terminating explorer.exe because the taskbar can interfere with cooperative fullscreen");
		::system("TASKKILL /F /IM explorer.exe");
	}
}

void sleep()
{
	oVB(SetSuspendState(FALSE, TRUE, FALSE));
}

void allow_sleep(bool allow)
{
	switch (windows::get_version())
	{
		case windows::version::win2000:
		case windows::version::xp:
		case windows::version::xp_pro_64bit:
		case windows::version::server_2003:
		case windows::version::server_2003r2:
			throw std::system_error(std::errc::operation_not_supported, std::system_category(), std::string("allow_sleep not supported on ") + as_string(windows::get_version()));
		default:
			break;
	}

	EXECUTION_STATE next = allow ? ES_CONTINUOUS : (ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_AWAYMODE_REQUIRED);
	oVB(!SetThreadExecutionState(next));
}

typedef void (*on_timer_fn)(void* user);

struct SCHEDULED_FUNCTION_CONTEXT
{
	HANDLE      htimer;
	on_timer_fn on_timer;
	void*       user;
	time_t      scheduled_time;
	sstring     debug_name;
};

static void CALLBACK execute_scheduled_function_and_cleanup(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
	SCHEDULED_FUNCTION_CONTEXT& ctx = *(SCHEDULED_FUNCTION_CONTEXT*)lpArgToCompletionRoutine;
	if (ctx.on_timer)
	{
		#ifdef _DEBUG
			sstring diff;
			format_duration(diff, (double)(time(nullptr) - ctx.scheduled_time));
			oTRACE("Running scheduled function '%s' %s after it was scheduled", ctx.debug_name.empty() ? "(null)" : ctx.debug_name.c_str(), diff.c_str());
		#endif

		ctx.on_timer(ctx.user);
		oTRACE("Finished scheduled function '%s'", ctx.debug_name.empty() ? "(null)" : ctx.debug_name.c_str());
	}
	oVB(CloseHandle(ctx.htimer));
	delete &ctx;
}

static void schedule_task(const char* debug_name
	, time_t absolute_time
	, bool alertable
	, on_timer_fn on_timer, void* user)
{
	SCHEDULED_FUNCTION_CONTEXT& ctx = *new SCHEDULED_FUNCTION_CONTEXT();
	ctx.htimer = CreateWaitableTimer(nullptr, TRUE, nullptr);
	if (!ctx.htimer)
		throw windows::error();
	ctx.on_timer = on_timer;
	ctx.user = user;
	ctx.scheduled_time = absolute_time;

	if (oSTRVALID(debug_name))
		strlcpy(ctx.debug_name, debug_name);
	
	#ifdef _DEBUG
		date then;
		char str_time[64];
		char str_diff[64];
		try { then = date_cast<date>(absolute_time); }
		catch (std::exception&) { strlcpy(str_time, "(out-of-time_t-range)"); }
		strftime(str_time, sortable_date_format, then);
		format_duration(str_diff, (double)(time(nullptr) - ctx.scheduled_time));
		oTRACE("Setting timer to run function '%s' at %s (%s from now)", oSAFESTRN(ctx.debug_name), str_time, str_diff);
	#endif

	FILETIME ft = date_cast<FILETIME>(absolute_time);
	LARGE_INTEGER liDueTime;
	liDueTime.LowPart = ft.dwLowDateTime;
	liDueTime.HighPart = ft.dwHighDateTime;
	oVB(!SetWaitableTimer(ctx.htimer, &liDueTime, 0, execute_scheduled_function_and_cleanup, (LPVOID)&ctx, alertable ? TRUE : FALSE));
}

void schedule_wakeup(time_t time, on_wake_fn on_wake, void* user)
{
	schedule_task("Ouroboros.Wakeup", time, true, on_wake, user);
}

void wait_idle(continue_waiting_fn continue_waiting, void* user)
{
	wait_for_idle(INFINITE, continue_waiting, user);
}

typedef bool (*service_enumerator_fn)(SC_HANDLE hscmanager, const ENUM_SERVICE_STATUS_PROCESS& status, void* user);
void windows_enumerate_services(service_enumerator_fn enumerator, void* user)
{
	SC_HANDLE hscmanager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	oVB(hscmanager);
	oFinally { oVB(CloseServiceHandle(hscmanager)); };

	DWORD req_bytes = 0;
	DWORD nservices = 0;
	EnumServicesStatusEx(hscmanager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL, SERVICE_STATE_ALL, nullptr, 0, &req_bytes, &nservices, nullptr, nullptr);
	oVB(GetLastError() == ERROR_MORE_DATA);

	ENUM_SERVICE_STATUS_PROCESS* services = (ENUM_SERVICE_STATUS_PROCESS*)malloc(req_bytes);
	oFinally { free(services); };
	
	oVB(EnumServicesStatusEx(hscmanager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL, SERVICE_STATE_ALL, (LPBYTE)services, req_bytes, &req_bytes, &nservices, nullptr, nullptr));
	for (DWORD i = 0; i < nservices; i++)
		if (!enumerator(hscmanager, services[i], user))
			break;
}

static bool windows_all_services_steady()
{
	bool non_steady_state_found = false;

	windows_enumerate_services([](SC_HANDLE hscmanager, const ENUM_SERVICE_STATUS_PROCESS& status, void* user)->bool
	{
		bool& non_steady_state_found = *(bool*)user;

		switch (status.ServiceStatusProcess.dwCurrentState)
		{
			case SERVICE_PAUSED:
			case SERVICE_RUNNING:
			case SERVICE_STOPPED:
				break;
			default:
			{
				non_steady_state_found = true;
				return false;
			}
		}

		return true;
	}, &non_steady_state_found);

	return !non_steady_state_found;
}

static double windows_cpu_usage(uint64_t* out_prev_idle_time, uint64_t* out_prev_system_time)
{
	double CPUUsage = 0.0;

	uint64_t idle_time, kernel_time, user_time;
	oVB(GetSystemTimes((LPFILETIME)&idle_time, (LPFILETIME)&kernel_time, (LPFILETIME)&user_time));
	uint64_t systemTime = kernel_time + user_time;
	
	if (*out_prev_idle_time && *out_prev_system_time)
	{
		uint64_t idleDelta = idle_time - *out_prev_idle_time;
		uint64_t systemDelta = systemTime - *out_prev_system_time;
		CPUUsage = (systemDelta - idleDelta) * 100.0 / (double)systemDelta;
	}		

	*out_prev_idle_time = idle_time;
	*out_prev_system_time = systemTime;
	return CPUUsage; 
}

bool wait_for_idle(uint32_t timeout_ms, continue_waiting_fn continue_waiting, void* user)
{
	static const double kLowCPUUsage = 5.0;
	static const uint32_t kNumSamplesAtLowCPUUsageToBeSteady = 10;

	bool is_steady = false;
	auto time_start = std::chrono::high_resolution_clock::now();
	auto time_current = time_start;
	uint64_t prev_idle_time = 0, prev_system_time = 0;
	uint32_t num_samples_at_low_cpu_usage = 0;
	
	while (true)
	{
		if (timeout_ms != INFINITE && (std::chrono::seconds((time_current - time_start).count()) >= std::chrono::milliseconds(timeout_ms)))
			break;
		
		if (windows_all_services_steady())
		{
			double CPUUsage = windows_cpu_usage(&prev_idle_time, &prev_system_time);
			if (CPUUsage < kLowCPUUsage)
				num_samples_at_low_cpu_usage++;
			else
				num_samples_at_low_cpu_usage = 0;

			if (num_samples_at_low_cpu_usage > kNumSamplesAtLowCPUUsageToBeSteady)
				return true;

			if (continue_waiting && !continue_waiting(user))
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		time_current = std::chrono::high_resolution_clock::now();
	}

	return false;
}

bool uses_gpu_compositing()
{
	BOOL enabled = FALSE;
	oV(DwmIsCompositionEnabled(&enabled));
	return !!enabled;
}

void enable_gpu_compositing(bool enable, bool force)
{
	#if NTDDI_VERSION < _WIN32_WINNT_WIN8
		if (enable && force && !uses_gpu_compositing())
		{
			::system("%SystemRoot%\\system32\\rundll32.exe %SystemRoot%\\system32\\shell32.dll,Control_RunDLL %SystemRoot%\\system32\\desk.cpl desk,@Themes /Action:OpenTheme /file:\"C:\\Windows\\Resources\\Themes\\aero.theme\"");
			std::this_thread::sleep_for(std::chrono::seconds(31)); // Windows takes about 30 sec to settle after doing this.
		}
		else	
			oVB(DwmEnableComposition(enable ? DWM_ECenableCOMPOSITION : DWM_EC_DISABLECOMPOSITION));
	#else
		throw std::system_error(std::errc::function_not_supported, std::system_category(), "No programatic control of GPU compositing in Windows 8 and beyond.");
	#endif
}

bool is_remote_session()
{
	return !!GetSystemMetrics(SM_REMOTESESSION);
}

bool gui_is_drawable()
{
	return !!GetForegroundWindow();
}

char* operating_system_name(char* dst, size_t dst_size)
{
	const char* OSName = as_string(windows::get_version());
	if (strlcpy(dst, OSName, dst_size) >= dst_size)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return dst;
}

char* host_name(char* dst, size_t dst_size)
{
	DWORD nElements = (DWORD)dst_size;
	oVB(GetComputerNameExA(ComputerNameDnsHostname, dst, &nElements));
	return dst;
}

char* workgroup_name(char* dst, size_t dst_size)
{
	LPWKSTA_INFO_102 pInfo = nullptr;
	NET_API_STATUS nStatus;
	LPTSTR pszServerName = nullptr;

	nStatus = NetWkstaGetInfo(nullptr, 102, (LPBYTE *)&pInfo);
	oFinally { if (pInfo) NetApiBufferFree(pInfo); };
	oVB(nStatus == NERR_Success);
	
	WideCharToMultiByte(CP_ACP, 0, pInfo->wki102_langroup, -1, dst, static_cast<int>(dst_size), 0, 0);
	return dst;
}

char* exec_path(char* dst, size_t dst_size)
{
	sstring hostname;
	if (-1 == snprintf(dst, dst_size, "[%s.%u.%u]", host_name(hostname), ouro::this_process::get_id(), asdword(std::this_thread::get_id())))
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return dst;
}

void setenv(const char* envvar_name, const char* value)
{
	oVB(SetEnvironmentVariableA(envvar_name, value));
}

char* getenv(char* value, size_t value_size, const char* envvar_name)
{
	size_t len = GetEnvironmentVariableA(envvar_name, value, (int)value_size);
	return (len && len < value_size) ? value : nullptr;
}

char* envstr(char* env_string, size_t env_string_size)
{
	char* pEnv = GetEnvironmentStringsA();
	oFinally { if (pEnv) { FreeEnvironmentStringsA(pEnv); } };

	// nuls -> newlines to make parsing this mega-string a bit less obtuse
	char* c = pEnv;
	char* d = env_string;
	size_t len = strlen(pEnv);
	while (len)
	{
		c[len] = '\n';
		c += len+1;
		len = strlen(c);
	}

	if (strlcpy(env_string, pEnv, env_string_size) >= env_string_size)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return env_string;
}

static const char* privilege_name(privilege privilege)
{
	switch (privilege)
	{
		case privilege::may_debug: return SE_DEBUG_NAME;
		case privilege::may_shutdown: return SE_SHUTDOWN_NAME;
		case privilege::may_lock_memory: return SE_LOCK_MEMORY_NAME;
		case privilege::may_profile_process: return SE_PROF_SINGLE_PROCESS_NAME;
		case privilege::may_load_driver: return SE_LOAD_DRIVER_NAME;
		case privilege::may_increase_working_set: return SE_INC_WORKING_SET_NAME;
		case privilege::may_increase_process_priority: return SE_INC_BASE_PRIORITY_NAME;
		default: break;
	}
	return "?";
}

void set_privilege(privilege privilege, bool enabled)
{
	const char* p = privilege_name(privilege);

	HANDLE hT;
	TOKEN_PRIVILEGES tkp;
	oVB(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hT));
	windows::scoped_handle hToken(hT);
	oVB(!LookupPrivilegeValue(nullptr, p, &tkp.Privileges[0].Luid));
	tkp.PrivilegeCount = 1;      
	tkp.Privileges[0].Attributes = enabled ? SE_PRIVILEGE_ENABLED : 0; 
	oVB(!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, 0));
	// AdjustTokenPrivileges only fails for invalid parameters. If it fails 
	// because the app did not have permissions, then it succeeds (result will be 
	// true), but sets last error to ERROR_NOT_ALL_ASSIGNED.
	oVB(GetLastError() == ERROR_SUCCESS);
}

// This moves any leftovers to the front of the buffer and returns the offset
// where a new write should start.
static size_t line_enumerator(char* buffer, size_t read_size, get_line_fn get_line, void* user)
{
	buffer[read_size] = '\0';
	char* line = buffer;
	size_t eol = strcspn(line, oNEWLINE);

	while (line[eol] != '\0')
	{
		line[eol] = '\0';
		get_line(line, user);
		line += eol + 1;
		line += strspn(line, oNEWLINE);
		eol = strcspn(line, oNEWLINE);
	}

	return strlcpy(buffer, line, read_size);
}

// starting suspended can BSOD the machine, be careful
//#define DEBUG_EXECUTED_PROCESS
int spawn_for(const char* cmdline
	, get_line_fn get_line
	, void* user
	, bool show_window
	, uint32_t execution_timeout_ms)
{
	std::vector<char> buffer;
	buffer.resize(100 * 1024);
	std::shared_ptr<process> P;
	{
		process::info process_info;
		process_info.command_line = cmdline;
		process_info.environment = nullptr;
		process_info.initial_directory = nullptr;
		process_info.stdout_buffer_size = buffer.size() - 1;
		process_info.show = show_window ? process::show : process::hide;
		#ifdef DEBUG_EXECUTED_PROCESS
			process_info.suspended = true;
		#endif
		P = std::move(process::make(process_info));
	}

	oTRACEA("spawn: >>> %s <<<", oSAFESTRN(cmdline));
	#ifdef DEBUG_EXECUTED_PROCESS
		process->start();
	#endif

	// need to flush stdout once in a while or it can hang the process if we are 
	// redirecting output
	bool once = false;

	static const uint32_t ktimeoutPerFlushMS = 50;
	double timeout = execution_timeout_ms == INFINITE ? DBL_MAX : (1000.0 * static_cast<double>(execution_timeout_ms));
	double timeout_so_far = 0.0;
	timer t;
	do
	{
		if (!once)
		{
			oTRACEA("spawn stdout: >>> %s <<<", oSAFESTRN(cmdline));
			once = true;
		}
		
		size_t read_size = P->from_stdout((void*)buffer.data(), buffer.size() - 1);
		while (read_size && get_line)
		{
			size_t offset = line_enumerator(buffer.data(), read_size, get_line, user);
			read_size = P->from_stdout((uint8_t*)buffer.data() + offset, buffer.size() - 1 - offset);
		}

		timeout_so_far = t.seconds();

	} while (timeout_so_far < timeout && !P->wait_for(std::chrono::milliseconds(ktimeoutPerFlushMS)));
	
	// get any remaining text from stdout
	size_t offset = 0;
	size_t read_size = P->from_stdout((void*)buffer.data(), buffer.size() - 1);
	while (read_size && get_line)
	{
		offset = line_enumerator(buffer.data(), read_size, get_line, user);
		read_size = P->from_stdout((uint8_t*)buffer.data() + offset, buffer.size() - 1 - offset);
	}

	if (offset && get_line)
		get_line(buffer.data(), user);

	if (timeout_so_far >= timeout)
		return std::errc::timed_out;

	int exit_code = 0;
	if (!P->exit_code(&exit_code))
		exit_code = std::errc::operation_in_progress;
	
	return exit_code;
}

int spawn(const char* cmdline
	, get_line_fn get_line
	, void* user
	, bool show_window)
{
	return spawn_for(cmdline, get_line, user, show_window, INFINITE);
}

void spawn_associated_application(const char* document_path, bool for_edit)
{
	int hr = (int)ShellExecuteA(nullptr, for_edit ? "edit" : "open", document_path, nullptr, nullptr, SW_SHOW);
	if (hr < 32)
		throw std::system_error(std::errc::no_buffer_space, std::system_category(), "The operating system is out of memory or resources.");
}

}}
