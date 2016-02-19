// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// API for dealing with system-wide functionality

#pragma once
#include <oBase/date.h>
#include <cstdint>

namespace ouro { namespace system {

enum class privilege
{
	may_debug,
	may_shutdown,
	may_lock_memory,
	may_profile_process,
	may_load_driver,
	may_increase_working_set,
	may_increase_process_priority,
};

struct heap_info
{
	uint64_t total_used;
	uint64_t avail_physical;
	uint64_t total_physical;
	uint64_t avail_virtual_process;
	uint64_t total_virtual_process;
	uint64_t avail_paged;
	uint64_t total_paged;
};

heap_info get_heap_info();

// get the system's impression of current date/time in UTC (GMT).
template<typename dateT> void now(dateT* out_date);

// convert from a local timezone-specific time to UTC (GMT) time.
date from_local(const date& local);

// convert from UTC (GMT) time to a local timezone-specific time.
date to_local(const date& utc);

template<typename date1T, typename date2T> bool from_local(const date1T& local, date2T* out_utc)
{
	date loc, utc;
	loc = date_cast<date>(local);
	if (!from_local(loc, &utc)) return false;
	*out_utc = date_cast<date2T>(utc);
	return true;
}

template<typename date1T, typename date2T> bool to_local(const date1T& utc, date2T* out_local)
{
	date local, utc_date;
	utc_date = date_cast<date>(utc);
	if (!to_local(utc_date, &local)) return false;
	*out_local = date_cast<date2T>(local);
	return true;
}

// All OS rules apply, so this is a request that UI may have an active user or 
// application deny.
void reboot();
void shutdown();
void sleep();
void kill_file_browser();
void allow_sleep(bool allow = true);

typedef void (*on_wake_fn)(void* user);
void schedule_wakeup(time_t time, on_wake_fn on_wake, void* user);

// Poll system for all processes to be relatively idle (i.e. <3% CPU usage). 
// This is primarily intended to determine heuristically when a computer is 
// "fully booted" meaning all system services and startup applications are done.
// (This arose from Windows apps stealing focus during startup and the 
// application at the time needed to be a startup app and go into fullscreen.
// Randomly, another startup app/service would steal focus, knocking our app out
// of fullscreen mode.)
// continue_waiting is an optional function called while waiting. If it returns 
// false the wait will terminate.
typedef bool (*continue_waiting_fn)(void* user);
void wait_idle(continue_waiting_fn continue_waiting = nullptr, void* user = nullptr);
bool wait_for_idle(uint32_t timeout_ms, continue_waiting_fn continue_waiting = nullptr, void* user = nullptr);

bool uses_gpu_compositing();
void enable_gpu_compositing(bool enable = true, bool force = false);

// Returns true if the system is running in a mode similar to Window's 
// Remote Desktop. Use this when interacting with displays or feature set that
// might not be accurate by the software emulation done to redirect output to a 
// remote session.
bool is_remote_session();

// If the system is logged out, GUI draw commands will fail
bool gui_is_drawable();

char* operating_system_name(char* dst, size_t dst_size);
template<size_t size> char* operating_system_name(char (&dst)[size]) { return operating_system_name(dst, size); }
template<size_t capacity> char* operating_system_name(fixed_string<char, capacity>& dst) { return operating_system_name(dst, dst.capacity()); }

char* host_name(char* dst, size_t dst_size);
template<size_t size> char* host_name(char (&dst)[size]) { return host_name(dst, size); }
template<size_t capacity> char* host_name(fixed_string<char, capacity>& dst) { return host_name(dst, dst.capacity()); }

char* workgroup_name(char* dst, size_t dst_size);
template<size_t size> char* workgroup_name(char (&dst)[size]) { return workgroup_name(dst, size); }
template<size_t capacity> char* workgroup_name(fixed_string<char, capacity>& dst) { return workgroup_name(dst, dst.capacity()); }

// fills destination with the string "[<hostname>.process_id.thread_id]"
char* exec_path(char* dst, size_t dst_size);
template<size_t size> char* exec_path(char (&dst)[size]) { return exec_path(dst, size); }
template<size_t capacity> char* exec_path(fixed_string<char, capacity>& dst) { return exec_path(dst, dst.capacity()); }

void setenv(const char* envvar_name, const char* value);
char* getenv(char* value, size_t value_size, const char* envvar_name);
template<size_t size> char* getenv(char (&value)[size], const char* envvar_name) { return getenv(value, size, envvar_name); }
template<size_t capacity> char* getenv(fixed_string<char, capacity>& value, const char* envvar_name) { return getenv(value, value.capacity(), envvar_name); }

// retrieve entire environment string
char* envstr(char* env_string, size_t env_string_size);
template<size_t size> char* envstr(char (&env_string)[size]) { return envstr(env_string, size); }
template<size_t capacity> char* envstr(fixed_string<char, capacity>& env_string) { return envstr(env_string, env_string.capacity()); }

// Set the specified privilege for this process, if allowed by the system.
void set_privilege(privilege privilege, bool enabled = true);

// Spawns a child process to execute the specified command line. For each line
// emitted to stdout by the process, _GetLine is called so this calling process
// can react to the child's output. This returns the exit code of the process,
// or if the timeout is reached, this will return std::errc::timed_out. This 
// can also return std::errc::operation_in_process if the process did not time 
// out but still is not ready to return an exit code.

typedef void (*get_line_fn)(char* line, void* user);

int spawn_for(const char* cmdline
	, get_line_fn get_line
	, void* user
	, bool show_window
	, uint32_t execution_timeout_ms);

template<typename Rep, typename Period>
int spawn_for(const char* cmdline
	, get_line_fn get_line
	, void* user
	, bool show_window
	, const std::chrono::duration<Rep, Period>& _timeout)
{
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(_timeout);
	return spawn_for(cmdline, _GetLine, show_window, static_cast<uint32_t>(ms.count()));
}

// Same as above, but no timeout
int spawn(const char* cmdline
	, get_line_fn get_line
	, void* user
	, bool show_window);

// Spawns the application associated with the specified document_name by the 
// operating system and opens or edits that document.
void spawn_associated_application(const char* document_path, bool for_edit = false);

}}
