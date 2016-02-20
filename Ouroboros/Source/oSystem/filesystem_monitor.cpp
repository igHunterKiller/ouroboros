// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/filesystem_monitor.h>
#include <oSystem/filesystem.h>
#include <oConcurrency/mutex.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_iocp.h>
#include <oConcurrency/backoff.h>
#include <atomic>

#include <oBase/container_support.h>

namespace ouro {

template<> const char* as_string(const filesystem::file_event& e)
{
	static const char* s_names[] =
	{
		"unsupported",
		"added",
		"removed",
		"modified",
		"accessible",
	};
	return as_string(e, s_names);
}

	namespace filesystem {

static file_event as_event(DWORD notify_action)
{
	switch (notify_action)
	{
		case FILE_ACTION_ADDED:            return file_event::added;
		case FILE_ACTION_REMOVED:          return file_event::removed;
		case FILE_ACTION_MODIFIED:         return file_event::modified;
		case FILE_ACTION_RENAMED_OLD_NAME: return file_event::removed;
		case FILE_ACTION_RENAMED_NEW_NAME: return file_event::added;
		default: break;
	}
	return file_event::unsupported;
}

static bool is_accessible(const char* path)
{
	// returns true if the file exists and can be opened for shared reading
	if (!path)
		return false;

	DWORD dwAttributes = GetFileAttributes(path);
	if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	bool accessible = false;
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		accessible = true;
		oVB(CloseHandle(hFile));
	}

	return accessible;
}

class monitor_impl;
class watcher
{
public:
	watcher(monitor_impl* monitor, const path_t& path, bool recursive, size_t buffer_size);
	~watcher();

	void watch_changes();
	void unwatch_changes();
	bool watching(const path_t& path) const;

private:
	mutex process_mutex_;
	void* buffer_[2];
	DWORD buffer_size_;
	monitor_impl* monitor_;
	OVERLAPPED* overlapped_;
	HANDLE hdirectory_;
	path_t directory_;
	sstring filename_;
	bool recursive_;
	bool watching_;
	char buffer_index_;

	void process(FILE_NOTIFY_INFORMATION* notify);

	static void static_on_complete(void* context, uint64_t num_bytes) { ((watcher*)context)->on_complete(num_bytes); }
	void on_complete(uint64_t num_bytes);
};

class monitor_impl : public monitor
{
public:
	monitor_impl(const info& info, on_event_fn on_event, void* user);
	~monitor_impl();
	info get_info() const override { return info_; }
  void watch(const path_t& path, size_t buffer_size, bool recursive) override;
  void unwatch(const path_t& path) override;
	void unwatch_all();

	// API called from class watch
	void watch_ended() { num_active_watches_--; }
	void on_event(const path_t& path, file_event _Event, double _Timestamp);

private:

	on_event_fn on_event_;
	void* user_;
	mutex watches_mutex_;
	std::vector<watcher*> watches_;
	std::vector<path_t> accessibles_;

	HANDLE htimer_queue_;
	HANDLE htimer_queue_timer_;
	info info_;
	std::atomic<int> num_active_watches_;
	bool debug_;

	// Some events need further analysis, so here's a container for them
	// to hold over from the immediate event until they can be looked at.

	struct EVENT
	{
		double timestamp;
		file_event event;
	};

	mutex events_mutex_;
	std::map<path_t, EVENT, less_i<path_t>> events_;
	mutex accessibility_mutex_;

	void check_accessibility();
	static VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired) { static_cast<monitor_impl*>(lpParameter)->check_accessibility(); }
};

watcher::watcher(monitor_impl* monitor, const path_t& path, bool recursive, size_t buffer_size)
	: monitor_(monitor)
	, overlapped_(nullptr)
	, hdirectory_(INVALID_HANDLE_VALUE)
	, directory_(path)
	, buffer_size_(static_cast<DWORD>(buffer_size))
	, buffer_index_(0)
	, recursive_(recursive)
	, watching_(true)
{
	if (directory_.has_filename())
	{
		oCheck(recursive, std::errc::invalid_argument, "a filename/wildcard cannot be recursive");
		filename_ = directory_.filename().c_str();
		directory_.remove_filename();
	}

	if (!exists(directory_))
		create_directory(directory_);

	oCheck(exists(directory_), std::errc::no_such_file_or_directory, "direction does not exist: %s", directory_.c_str());
	oCheck(is_directory(directory_), std::errc::not_a_directory, "not a directory: %s", directory_.c_str());

	hdirectory_ = CreateFile(directory_
		, FILE_LIST_DIRECTORY
		, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
		, nullptr
		, OPEN_EXISTING
		, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED
		, nullptr);

	oVB(hdirectory_ != INVALID_HANDLE_VALUE);

	overlapped_ = windows::iocp::associate(hdirectory_, watcher::static_on_complete, this);

	oCheck(overlapped_, std::errc::operation_would_block, "OVERLAPPED allocation failed");

	buffer_[0] = new char[buffer_size];
	buffer_[1] = new char[buffer_size];
}

watcher::~watcher()
{
	lock_guard<mutex> lock(process_mutex_);

	if (hdirectory_ != INVALID_HANDLE_VALUE)
		CloseHandle(hdirectory_);

	windows::iocp::disassociate(overlapped_);

	if (buffer_[0])
		delete [] (char*)buffer_[0];

	if (buffer_[1])
		delete [] (char*)buffer_[1];

	monitor_->watch_ended();
}

void watcher::watch_changes()
{
	oVB(ReadDirectoryChangesW(hdirectory_
				, buffer_[buffer_index_]
				, static_cast<DWORD>(buffer_size_)
				, recursive_
				, FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_FILE_NAME
				, nullptr
				, overlapped_
				, nullptr));
}

void watcher::unwatch_changes()
{
	{
		lock_guard<mutex> lock(process_mutex_);
		watching_ = false;
	}
	CancelIoEx(hdirectory_, overlapped_);
}

void watcher::on_complete(uint64_t num_bytes)
{
	bool delete_this = num_bytes == 0;
	if (num_bytes)
	{
		process_mutex_.lock();
		int OldIndex = buffer_index_;
		buffer_index_ = (buffer_index_ + 1) & 0x1;
		if (watching_)
			watch_changes();
		else
			delete_this = true;
		process((FILE_NOTIFY_INFORMATION*)buffer_[OldIndex]);
	}
	
	if (delete_this)
	{
		monitor_impl* m = monitor_;
		delete this;
	}
}

void watcher::process(FILE_NOTIFY_INFORMATION* notify)
{
	FILE_NOTIFY_INFORMATION* n = notify;
	double Now = timer::now();
	while (true)
	{
		file_event e = as_event(n->Action);
		if (e != file_event::unsupported)
		{
			// *2 for that fact that FileNameLength is # bytes for a wide-char non-
			// null-term string
			oCheck(n->FileNameLength < (path_string::Capacity*2), std::errc::no_buffer_space, "not expecting paths that are longer than they should be");

			// note the path given to us by windows is only the portion after the 
			// path of the folder we are monitoring, so have to reform the full path.

			// These strings aren't null-terminated, AND in wide-character so convert 
			// back to a useable form.
			path_t p(directory_);
			{
				wchar_t ForNullTermination[path_string::Capacity];
				memcpy(ForNullTermination, n->FileName, n->FileNameLength);
				ForNullTermination[n->FileNameLength / 2] = 0;
				path_string pathString(ForNullTermination);
				p /= pathString;
			}

			if (!filename_ || !*filename_ || matches_wildcard(filename_, p.c_str()))
				monitor_->on_event(p, e, Now);
		}

		if (!n->NextEntryOffset)
			break;

		n = (FILE_NOTIFY_INFORMATION*)((uint8_t*)n + n->NextEntryOffset);
	}

	process_mutex_.unlock();
}

bool watcher::watching(const path_t& path) const
{
	path_t that_dir(path);
	sstring that_filename;

	if (!filename_.empty())
	{
		that_filename = that_dir.filename().c_str();
		that_dir.remove_filename();
	}

	return same_i<path_t>()(directory_, that_dir) && same_i<sstring>()(filename_, that_filename);
}

std::shared_ptr<monitor> monitor::make(const info& info, on_event_fn on_event, void* user)
{
	return std::make_shared<monitor_impl>(info, on_event, user);
}

monitor_impl::monitor_impl(const info& info, on_event_fn on_event, void* user)
		: info_(info)
		, on_event_(on_event)
		, user_(user)
		, htimer_queue_(CreateTimerQueue())
		, htimer_queue_timer_(nullptr)
		, num_active_watches_(false)
{
	oVB(htimer_queue_);
	HANDLE hTimer = nullptr;
	oVB(CreateTimerQueueTimer(
	&htimer_queue_timer_
	, htimer_queue_
	, monitor_impl::WaitOrTimerCallback
	, this
	, info.accessibility_poll_rate_ms
	, info.accessibility_poll_rate_ms
	, WT_EXECUTEDEFAULT
	));

	accessibles_.reserve(64);
}

monitor_impl::~monitor_impl()
{
	size_t nWatches = watches_.size();
	unwatch_all();
	
	backoff bo;
	while (num_active_watches_)
		bo.pause();

	HANDLE hEvent = CreateEventA(0, FALSE, FALSE, nullptr);
	ResetEvent(hEvent);
	oVB_NOTHROW(DeleteTimerQueueTimer(htimer_queue_, htimer_queue_timer_, hEvent));

	HRESULT hr = WaitForSingleObject(hEvent, INFINITE);
	oAssertA(hr == WAIT_OBJECT_0, "failed waiting for DeleteTimerQueueTimer");

	oVB_NOTHROW(CloseHandle(hEvent));
	oVB_NOTHROW(DeleteTimerQueue(htimer_queue_));
}

void monitor_impl::on_event(const path_t& path, file_event _Event, double _Timestamp)
{
	// Files that are added or modified aren't necessarily usable, so queue
	// such files up for later analysis for accessibility.
	if (_Event == file_event::added || _Event == file_event::modified)
	{
		EVENT e;
		e.timestamp = _Timestamp;
		e.event = _Event;
		lock_guard<mutex> lock(events_mutex_);
		events_[path] = e;
	}

	if (on_event_)
		on_event_(_Event, path, user_);
}

void monitor_impl::check_accessibility()
{
	lock_guard<mutex> lock(accessibility_mutex_);

	if (events_.empty())
		return;

	const double Timeout = info_.accessibility_timeout_ms / 1000.0;
	const double Now = timer::now();

	{
		lock_guard<mutex> lock_events(events_mutex_);
		for (auto it = std::begin(events_); it != std::end(events_); /* no increment */)
		{
			const path_t& p = it->first;
			const EVENT& e = it->second;

			if (is_accessible(p))
			{
				accessibles_.push_back(p);
				it = events_.erase(it);
			}

			else if ((e.timestamp + Timeout) > Now)
			{
				oTraceA("monitor: accessibility for %s timed out", p.c_str());
				it = events_.erase(it);
			}

			else
				++it;
		}
	}

	for (const auto& a : accessibles_)
		if (on_event_)
			on_event_(file_event::accessible, a, user_);

	accessibles_.clear();
}

void monitor_impl::watch(const path_t& path, size_t buffer_size, bool recursive)
{
	lock_guard<mutex> lock(watches_mutex_);

	for (auto it = std::begin(watches_); it != std::end(watches_); ++it)
		oCheck(!(*it)->watching(path), std::errc::operation_in_progress, "already watching %s", path.c_str());

	watches_.push_back(new watcher(this, path, recursive, buffer_size));
	watches_.back()->watch_changes();

	num_active_watches_++;
}

void monitor_impl::unwatch(const path_t& path)
{
	lock_guard<mutex> lock(watches_mutex_);
	for (auto it = std::begin(watches_); it != std::end(watches_); /* no inc */)
	{
		if ((*it)->watching(path))
		{
			(*it)->unwatch_changes();
			delete *it;
			it = watches_.erase(it);
		}

		else
			++it;
	}
}

void monitor_impl::unwatch_all()
{
	lock_guard<mutex> lock(watches_mutex_);
	for (auto w : watches_)
		w->unwatch_changes();
	watches_.clear();
}

}}
