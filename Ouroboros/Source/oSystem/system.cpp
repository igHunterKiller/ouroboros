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

using namespace std;

namespace ouro {
	namespace system {

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

static void now(FILETIME* _pFileTime, bool _IsUTC)
{
	if (_IsUTC)
		GetSystemTimeAsFileTime(_pFileTime);
	else
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		SystemTimeToFileTime(&st, _pFileTime);
	}
}

void now(ntp_timestamp* _pNTPTimestamp)
{
	FILETIME ft;
	now(&ft, true);
	*_pNTPTimestamp = date_cast<ntp_timestamp>(ft);
}

void now(ntp_date* _pNTPDate)
{
	FILETIME ft;
	now(&ft, true);
	*_pNTPDate = date_cast<ntp_date>(ft);
}

void now(date* _pDate)
{
	FILETIME ft;
	now(&ft, true);
	*_pDate = date_cast<date>(ft);
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

date from_local(const date& _local_date)
{
	SYSTEMTIME In, Out;
	from_date(_local_date, &In);
	oVB(!SystemTimeToTzSpecificLocalTime(nullptr, &In, &Out));
	return to_date(Out);
}

date to_local(const date& _UTCDate)
{
	SYSTEMTIME In, Out;
	from_date(_UTCDate, &In);
	oVB(!SystemTimeToTzSpecificLocalTime(nullptr, &In, &Out));
	return to_date(Out);
}

void reboot()
{
	set_privilege(may_shutdown);
	oVB(ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_FLAG_PLANNED));
}

void shutdown()
{
	set_privilege(may_shutdown);
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

void allow_sleep(bool _Allow)
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

	EXECUTION_STATE next = _Allow ? ES_CONTINUOUS : (ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_AWAYMODE_REQUIRED);
	oVB(!SetThreadExecutionState(next));
}

struct SCHEDULED_FUNCTION_CONTEXT
{
	HANDLE hTimer;
	std::function<void()> OnTimer;
	time_t ScheduledTime;
	sstring DebugName;
};

static void CALLBACK ExecuteScheduledFunctionAndCleanup(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue)
{
	SCHEDULED_FUNCTION_CONTEXT& Context = *(SCHEDULED_FUNCTION_CONTEXT*)lpArgToCompletionRoutine;
	if (Context.OnTimer)
	{
		#ifdef _DEBUG
			sstring diff;
			format_duration(diff, (double)(time(nullptr) - Context.ScheduledTime));
			oTRACE("Running scheduled function '%s' %s after it was scheduled", oSAFESTRN(Context.DebugName), diff.c_str());
		#endif

		Context.OnTimer();
		oTRACE("Finished scheduled function '%s'", oSAFESTRN(Context.DebugName));
	}
	oVB(CloseHandle(Context.hTimer));
	delete &Context;
}

static void schedule_task(const char* _DebugName
	, time_t _AbsoluteTime
	, bool _Alertable
	, const std::function<void()>& _Task)
{
	SCHEDULED_FUNCTION_CONTEXT& Context = *new SCHEDULED_FUNCTION_CONTEXT();
	Context.hTimer = CreateWaitableTimer(nullptr, TRUE, nullptr);
	if (!Context.hTimer)
		throw windows::error();
	Context.OnTimer = _Task;
	Context.ScheduledTime = _AbsoluteTime;

	if (oSTRVALID(_DebugName))
		strlcpy(Context.DebugName, _DebugName);
	
	#ifdef _DEBUG
		date then;
		char StrTime[64];
		char StrDiff[64];
		try { then = date_cast<date>(_AbsoluteTime); }
		catch (std::exception&) { strlcpy(StrTime, "(out-of-time_t-range)"); }
		strftime(StrTime, sortable_date_format, then);
		format_duration(StrDiff, (double)(time(nullptr) - Context.ScheduledTime));
		oTRACE("Setting timer to run function '%s' at %s (%s from now)", oSAFESTRN(Context.DebugName), StrTime, StrDiff);
	#endif

	FILETIME ft = date_cast<FILETIME>(_AbsoluteTime);
	LARGE_INTEGER liDueTime;
	liDueTime.LowPart = ft.dwLowDateTime;
	liDueTime.HighPart = ft.dwHighDateTime;
	oVB(!SetWaitableTimer(Context.hTimer, &liDueTime, 0, ExecuteScheduledFunctionAndCleanup, (LPVOID)&Context, _Alertable ? TRUE : FALSE));
}

void schedule_wakeup(time_t _Time, const std::function<void()>& _OnWake)
{
	schedule_task("Ouroboros.Wakeup", _Time, true, _OnWake);
}

void wait_idle(const std::function<bool()>& _ContinueWaiting)
{
	wait_for_idle(INFINITE, _ContinueWaiting);
}

void windows_enumerate_services(const std::function<bool(SC_HANDLE _hSCManager, const ENUM_SERVICE_STATUS_PROCESS& _Status)>& _Enumerator)
{
	SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	oVB(hSCManager);
	oFinally { oVB(CloseServiceHandle(hSCManager)); };

	DWORD requiredBytes = 0;
	DWORD nServices = 0;
	EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL, SERVICE_STATE_ALL, nullptr, 0, &requiredBytes, &nServices, nullptr, nullptr);
	oVB(GetLastError() == ERROR_MORE_DATA);

	ENUM_SERVICE_STATUS_PROCESS* lpServices = (ENUM_SERVICE_STATUS_PROCESS*)malloc(requiredBytes);
	oFinally { free(lpServices); };
	
	oVB(EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL, SERVICE_STATE_ALL, (LPBYTE)lpServices, requiredBytes, &requiredBytes, &nServices, nullptr, nullptr));
	for (DWORD i = 0; i < nServices; i++)
		if (!_Enumerator(hSCManager, lpServices[i]))
			break;
}

static bool windows_all_services_steady()
{
	bool NonSteadyStateFound = false;
	windows_enumerate_services([&](SC_HANDLE _hSCManager, const ENUM_SERVICE_STATUS_PROCESS& _Status)->bool
	{
		if (SERVICE_PAUSED != _Status.ServiceStatusProcess.dwCurrentState && SERVICE_RUNNING != _Status.ServiceStatusProcess.dwCurrentState && SERVICE_STOPPED != _Status.ServiceStatusProcess.dwCurrentState)
		{
			NonSteadyStateFound = true;
			return false;
		}
		return true;
	});
	return !NonSteadyStateFound;
}

static double windows_cpu_usage(unsigned long long* _pPreviousIdleTime, unsigned long long* _pPreviousSystemTime)
{
	double CPUUsage = 0.0;

	unsigned long long idleTime, kernelTime, userTime;
	oVB(GetSystemTimes((LPFILETIME)&idleTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime));
	unsigned long long systemTime = kernelTime + userTime;
	
	if (*_pPreviousIdleTime && *_pPreviousSystemTime)
	{
		unsigned long long idleDelta = idleTime - *_pPreviousIdleTime;
		unsigned long long systemDelta = systemTime - *_pPreviousSystemTime;
		CPUUsage = (systemDelta - idleDelta) * 100.0 / (double)systemDelta;
	}		

	*_pPreviousIdleTime = idleTime;
	*_pPreviousSystemTime = systemTime;
	return CPUUsage; 
}

bool wait_for_idle(unsigned int _TimeoutMS, const std::function<bool()>& _ContinueWaiting)
{
	bool IsSteady = false;
	auto TimeStart = chrono::high_resolution_clock::now();
	auto TimeCurrent = TimeStart;
	unsigned long long PreviousIdleTime = 0, PreviousSystemTime = 0;
	static const double kLowCPUUsage = 5.0;
	static const unsigned int kNumSamplesAtLowCPUUsageToBeSteady = 10;
	unsigned int nSamplesAtLowCPUUsage = 0;
	
	while (true)
	{
		if (_TimeoutMS != INFINITE && (chrono::seconds((TimeCurrent - TimeStart).count()) >= chrono::milliseconds(_TimeoutMS)))
			break;
		
		if (windows_all_services_steady())
		{
			double CPUUsage = windows_cpu_usage(&PreviousIdleTime, &PreviousSystemTime);
			if (CPUUsage < kLowCPUUsage)
				nSamplesAtLowCPUUsage++;
			else
				nSamplesAtLowCPUUsage = 0;

			if (nSamplesAtLowCPUUsage > kNumSamplesAtLowCPUUsageToBeSteady)
				return true;

			if (_ContinueWaiting && !_ContinueWaiting())
				break;

			this_thread::sleep_for(chrono::milliseconds(200));
		}

		TimeCurrent = chrono::high_resolution_clock::now();
	}

	return false;
}

bool uses_gpu_compositing()
{
	BOOL enabled = FALSE;
	oV(DwmIsCompositionEnabled(&enabled));
	return !!enabled;
}

void enable_gpu_compositing(bool _Enable, bool _Force)
{
	#if NTDDI_VERSION < _WIN32_WINNT_WIN8
		if (_Enable && _Force && !uses_gpu_compositing())
		{
			::system("%SystemRoot%\\system32\\rundll32.exe %SystemRoot%\\system32\\shell32.dll,Control_RunDLL %SystemRoot%\\system32\\desk.cpl desk,@Themes /Action:OpenTheme /file:\"C:\\Windows\\Resources\\Themes\\aero.theme\"");
			std::this_thread::sleep_for(std::chrono::seconds(31)); // Windows takes about 30 sec to settle after doing this.
		}
		else	
			oVB(DwmEnableComposition(_Enable ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION));
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

char* operating_system_name(char* _StrDestination, size_t _SizeofStrDestination)
{
	const char* OSName = as_string(windows::get_version());
	if (strlcpy(_StrDestination, OSName, _SizeofStrDestination) >= _SizeofStrDestination)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return _StrDestination;
}

char* host_name(char* _StrDestination, size_t _SizeofStrDestination)
{
	DWORD nElements = (DWORD)_SizeofStrDestination;
	oVB(GetComputerNameExA(ComputerNameDnsHostname, _StrDestination, &nElements));
	return _StrDestination;
}

char* workgroup_name(char* _StrDestination, size_t _SizeofStrDestination)
{
	LPWKSTA_INFO_102 pInfo = nullptr;
	NET_API_STATUS nStatus;
	LPTSTR pszServerName = nullptr;

	nStatus = NetWkstaGetInfo(nullptr, 102, (LPBYTE *)&pInfo);
	oFinally { if (pInfo) NetApiBufferFree(pInfo); };
	oVB(nStatus == NERR_Success);
	
	WideCharToMultiByte(CP_ACP, 0, pInfo->wki102_langroup, -1, _StrDestination, static_cast<int>(_SizeofStrDestination), 0, 0);
	return _StrDestination;
}

char* exec_path(char* _StrDestination, size_t _SizeofStrDestination)
{
	sstring hostname;
	if (-1 == snprintf(_StrDestination, _SizeofStrDestination, "[%s.%u.%u]", host_name(hostname), ouro::this_process::get_id(), asdword(std::this_thread::get_id())))
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return _StrDestination;
}

void setenv(const char* _EnvVarName, const char* _Value)
{
	oVB(SetEnvironmentVariableA(_EnvVarName, _Value));
}

char* getenv(char* _Value, size_t _SizeofValue, const char* _EnvVarName)
{
	size_t len = GetEnvironmentVariableA(_EnvVarName, _Value, (int)_SizeofValue);
	return (len && len < _SizeofValue) ? _Value : nullptr;
}

char* envstr(char* _StrEnvironment, size_t _SizeofStrEnvironment)
{
	char* pEnv = GetEnvironmentStringsA();
	oFinally { if (pEnv) { FreeEnvironmentStringsA(pEnv); } };

	// nuls -> newlines to make parsing this mega-string a bit less obtuse
	char* c = pEnv;
	char* d = _StrEnvironment;
	size_t len = strlen(pEnv);
	while (len)
	{
		c[len] = '\n';
		c += len+1;
		len = strlen(c);
	}

	if (strlcpy(_StrEnvironment, pEnv, _SizeofStrEnvironment) >= _SizeofStrEnvironment)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return _StrEnvironment;
}

// This moves any leftovers to the front of the buffer and returns the offset
// where a new write should start.
static size_t line_enumerator(char* _Buffer, size_t _ReadSize
	, const std::function<void(char* _Line)>& _GetLine)
{
	_Buffer[_ReadSize] = '\0';
	char* line = _Buffer;
	size_t eol = strcspn(line, oNEWLINE);

	while (line[eol] != '\0')
	{
		line[eol] = '\0';
		_GetLine(line);
		line += eol + 1;
		line += strspn(line, oNEWLINE);
		eol = strcspn(line, oNEWLINE);
	}

	return strlcpy(_Buffer, line, _ReadSize);
}

static const char* privilege_name(privilege _Privilege)
{
	switch (_Privilege)
	{
		case may_debug: return SE_DEBUG_NAME;
		case may_shutdown: return SE_SHUTDOWN_NAME;
		case may_lock_memory: return SE_LOCK_MEMORY_NAME;
		case may_profile_process: return SE_PROF_SINGLE_PROCESS_NAME;
		case may_load_driver: return SE_LOAD_DRIVER_NAME;
		case may_increase_working_set: return SE_INC_WORKING_SET_NAME;
		case may_increase_process_priority: return SE_INC_BASE_PRIORITY_NAME;
		default: break;
	}
	return "?";
}

void set_privilege(privilege _Privilege, bool _Enabled)
{
	const char* p = privilege_name(_Privilege);

	HANDLE hT;
	TOKEN_PRIVILEGES tkp;
	oVB(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hT));
	windows::scoped_handle hToken(hT);
	oVB(!LookupPrivilegeValue(nullptr, p, &tkp.Privileges[0].Luid));
	tkp.PrivilegeCount = 1;      
	tkp.Privileges[0].Attributes = _Enabled ? SE_PRIVILEGE_ENABLED : 0; 
	oVB(!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, 0));
	// AdjustTokenPrivileges only fails for invalid parameters. If it fails 
	// because the app did not have permissions, then it succeeds (result will be 
	// true), but sets last error to ERROR_NOT_ALL_ASSIGNED.
	oVB(GetLastError() == ERROR_SUCCESS);
}

// starting suspended can BSOD the machine, be careful
//#define DEBUG_EXECUTED_PROCESS
int spawn_for(const char* _CommandLine
	, const std::function<void(char* _Line)>& _GetLine
	, bool _ShowWindow
	, unsigned int _ExecutionTimeout)
{
	std::vector<char> buffer;
	buffer.resize(100 * 1024);
	std::shared_ptr<process> P;
	{
		process::info process_info;
		process_info.command_line = _CommandLine;
		process_info.environment = nullptr;
		process_info.initial_directory = nullptr;
		process_info.stdout_buffer_size = buffer.size() - 1;
		process_info.show = _ShowWindow ? process::show : process::hide;
		#ifdef DEBUG_EXECUTED_PROCESS
			process_info.suspended = true;
		#endif
		P = std::move(process::make(process_info));
	}

	oTRACEA("spawn: >>> %s <<<", oSAFESTRN(_CommandLine));
	#ifdef DEBUG_EXECUTED_PROCESS
		process->start();
	#endif

	// need to flush stdout once in a while or it can hang the process if we are 
	// redirecting output
	bool once = false;

	static const unsigned int kTimeoutPerFlushMS = 50;
	double Timeout = _ExecutionTimeout == INFINITE ? DBL_MAX : (1000.0 * static_cast<double>(_ExecutionTimeout));
	double TimeSoFar = 0.0;
	timer t;
	do
	{
		if (!once)
		{
			oTRACEA("spawn stdout: >>> %s <<<", oSAFESTRN(_CommandLine));
			once = true;
		}
		
		size_t ReadSize = P->from_stdout((void*)buffer.data(), buffer.size() - 1);
		while (ReadSize && _GetLine)
		{
			size_t offset = line_enumerator(buffer.data(), ReadSize, _GetLine);
			ReadSize = P->from_stdout((uint8_t*)buffer.data() + offset, buffer.size() - 1 - offset);
		}

		TimeSoFar = t.seconds();

	} while (TimeSoFar < Timeout && !P->wait_for(chrono::milliseconds(kTimeoutPerFlushMS)));
	
	// get any remaining text from stdout
	size_t offset = 0;
	size_t ReadSize = P->from_stdout((void*)buffer.data(), buffer.size() - 1);
	while (ReadSize && _GetLine)
	{
		offset = line_enumerator(buffer.data(), ReadSize, _GetLine);
		ReadSize = P->from_stdout((uint8_t*)buffer.data() + offset, buffer.size() - 1 - offset);
	}

	if (offset && _GetLine)
		_GetLine(buffer.data());

	if (TimeSoFar >= Timeout)
		return std::errc::timed_out;

	int ExitCode = 0;
	if (!P->exit_code(&ExitCode))
		ExitCode = std::errc::operation_in_progress;
	
	return ExitCode;
}

int spawn(const char* _CommandLine
	, const std::function<void(char* _Line)>& _GetLine
	, bool _ShowWindow)
{
	return spawn_for(_CommandLine, _GetLine, _ShowWindow, INFINITE);
}

void spawn_associated_application(const char* _DocumentName, bool _ForEdit)
{
	int hr = (int)ShellExecuteA(nullptr, _ForEdit ? "edit" : "open", _DocumentName, nullptr, nullptr, SW_SHOW);
	if (hr < 32)
		throw std::system_error(std::errc::no_buffer_space, std::system_category(), "The operating system is out of memory or resources.");
}

	} // namespace system
}
