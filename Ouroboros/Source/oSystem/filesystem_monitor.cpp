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

	template<> const char* as_string<filesystem::file_event>(const filesystem::file_event& e)
	{
		switch (e)
		{
			case filesystem::file_event::unsupported: return "unsupported";
			case filesystem::file_event::added:       return "added";
			case filesystem::file_event::removed:     return "removed";
			case filesystem::file_event::modified:    return "modified";
			case filesystem::file_event::accessible:  return "accessible";
			default: break;
		}
		return "?";
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

// Returns true if the file exists and can be opened for shared reading, false 
// otherwise.
static bool is_accessible(const char* _Path)
{
	DWORD dwAttributes = GetFileAttributes(_Path);
	if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	bool IsAccessible = false;
	HANDLE hFile = CreateFile(_Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		IsAccessible = true;
		oVB(CloseHandle(hFile));
	}

	return IsAccessible;
}

class monitor_impl;
class watcher
{
public:
	watcher(monitor_impl* _pMonitor, const path_t& _Path, bool _Recursive, size_t _BufferSize);
	~watcher();

	void watch_changes();
	void unwatch_changes();
	bool watching(const path_t& _Path) const;

private:
	mutex ProcessMutex;
	void* pBuffer[2];
	DWORD BufferSize;
	monitor_impl* pMonitor;
	OVERLAPPED* pOverlapped;
	HANDLE hDirectory;
	path_t Directory;
	sstring Filename;
	bool Recursive;
	bool Watching;
	char BufferIndex;

	void process(FILE_NOTIFY_INFORMATION* _pNotify);

	static void static_on_complete(void* _pContext, unsigned long long _NumBytes) { ((watcher*)_pContext)->on_complete(_NumBytes); }
	void on_complete(unsigned long long _NumBytes);
};

class monitor_impl : public monitor
{
public:
	monitor_impl(const info& _Info, const std::function<void (file_event _Event, const path_t& _Path)>& _OnEvent);
	~monitor_impl();
	info get_info() const override { return Info; }
  void watch(const path_t& _Path, size_t _BufferSize, bool _Recursive) override;
  void unwatch(const path_t& _Path) override;
	void unwatch_all();

	// API called from class watch
	void watch_ended() { NumActiveWatches--; }
	void on_event(const path_t& _Path, file_event _Event, double _Timestamp);

private:

	std::function<void (file_event _Event, const path_t& _Path)> OnEvent;
	mutex WatchesMutex;
	std::vector<watcher*> Watches;
	std::vector<path_t> Accessibles;

	HANDLE hTimerQueue;
	HANDLE hTimerQueueTimer;
	info Info;
	std::atomic<int> NumActiveWatches;
	bool Debug;

	// Some events need further analysis, so here's a container for them
	// to hold over from the immediate event until they can be looked at.

	struct EVENT
	{
		double timestamp;
		file_event event;
	};

	mutex EventsMutex;
	std::map<path_t, EVENT, less_i<path_t>> Events;
	mutex AccessibilityMutex;

	void check_accessibility();
	static VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired) { static_cast<monitor_impl*>(lpParameter)->check_accessibility(); }
};

watcher::watcher(monitor_impl* _pMonitor, const path_t& _Path, bool _Recursive, size_t _BufferSize)
	: pMonitor(_pMonitor)
	, pOverlapped(nullptr)
	, hDirectory(INVALID_HANDLE_VALUE)
	, Directory(_Path)
	, BufferSize(static_cast<DWORD>(_BufferSize))
	, BufferIndex(0)
	, Recursive(_Recursive)
	, Watching(true)
{
	if (Directory.has_filename())
	{
		if (_Recursive)
			oThrow(std::errc::invalid_argument, "a filename/wildcard cannot be recursive");

		Filename = Directory.filename().c_str();
		Directory.remove_filename();
	}

	if (!exists(Directory))
		create_directory(Directory);

	oCheck(exists(Directory), std::errc::no_such_file_or_directory, "direction does not exist: %s", Directory.c_str());
	oCheck(is_directory(Directory), std::errc::not_a_directory, "not a directory: %s", Directory.c_str());

	hDirectory = CreateFile(Directory
		, FILE_LIST_DIRECTORY
		, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
		, nullptr
		, OPEN_EXISTING
		, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED
		, nullptr);

	oVB(hDirectory != INVALID_HANDLE_VALUE);

	pOverlapped = windows::iocp::associate(hDirectory, watcher::static_on_complete, this);

	oCheck(pOverlapped, std::errc::operation_would_block, "OVERLAPPED allocation failed");

	pBuffer[0] = new char[_BufferSize];
	pBuffer[1] = new char[_BufferSize];
}

watcher::~watcher()
{
	lock_guard<mutex> lock(ProcessMutex);

	if (hDirectory != INVALID_HANDLE_VALUE)
		CloseHandle(hDirectory);

	windows::iocp::disassociate(pOverlapped);

	if (pBuffer[0])
		delete [] (char*)pBuffer[0];

	if (pBuffer[1])
		delete [] (char*)pBuffer[1];

	pMonitor->watch_ended();
}

void watcher::watch_changes()
{
	oVB(ReadDirectoryChangesW(hDirectory
				, pBuffer[BufferIndex]
				, static_cast<DWORD>(BufferSize)
				, Recursive
				, FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_FILE_NAME
				, nullptr
				, pOverlapped
				, nullptr));
}

void watcher::unwatch_changes()
{
	{
		lock_guard<mutex> lock(ProcessMutex);
		Watching = false;
	}
	CancelIoEx(hDirectory, pOverlapped);
}

void watcher::on_complete(unsigned long long _NumBytes)
{
	bool DeleteThis = _NumBytes == 0;
	if (_NumBytes)
	{
		ProcessMutex.lock();
		int OldIndex = BufferIndex;
		BufferIndex = (BufferIndex + 1) & 0x1;
		if (Watching)
			watch_changes();
		else
			DeleteThis = true;
		process((FILE_NOTIFY_INFORMATION*)pBuffer[OldIndex]);
	}
	
	if (DeleteThis)
	{
		monitor_impl* m = pMonitor;
		delete this;
	}
}

void watcher::process(FILE_NOTIFY_INFORMATION* _pNotify)
{
	FILE_NOTIFY_INFORMATION* n = _pNotify;
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
			path_t p(Directory);
			{
				wchar_t ForNullTermination[path_string::Capacity];
				memcpy(ForNullTermination, n->FileName, n->FileNameLength);
				ForNullTermination[n->FileNameLength / 2] = 0;
				path_string pathString(ForNullTermination);
				p /= pathString;
			}

			if (!Filename || !*Filename || matches_wildcard(Filename, p.c_str()))
				pMonitor->on_event(p, e, Now);
		}

		if (!n->NextEntryOffset)
			break;

		n = (FILE_NOTIFY_INFORMATION*)((uint8_t*)n + n->NextEntryOffset);
	}

	ProcessMutex.unlock();
}

bool watcher::watching(const path_t& _Path) const
{
	path_t ThatDirectory(_Path);
	sstring ThatFilename;

	if (!Filename.empty())
	{
		ThatFilename = ThatDirectory.filename().c_str();
		ThatDirectory.remove_filename();
	}

	return same_i<path_t>()(Directory, ThatDirectory) && same_i<sstring>()(Filename, ThatFilename);
}

std::shared_ptr<monitor> monitor::make(const info& _Info, const std::function<void (file_event _Event, const path_t& _Path)>& _OnEvent)
{
	return std::make_shared<monitor_impl>(_Info, _OnEvent);
}

monitor_impl::monitor_impl(const info& _Info, const std::function<void (file_event _Event, const path_t& _Path)>& _OnEvent)
		: Info(_Info)
		, OnEvent(_OnEvent)
		, hTimerQueue(CreateTimerQueue())
		, hTimerQueueTimer(nullptr)
		, NumActiveWatches(false)
{
	oVB(hTimerQueue);
	HANDLE hTimer = nullptr;
	oVB(CreateTimerQueueTimer(
	&hTimerQueueTimer
	, hTimerQueue
	, monitor_impl::WaitOrTimerCallback
	, this
	, _Info.accessibility_poll_rate_ms
	, _Info.accessibility_poll_rate_ms
	, WT_EXECUTEDEFAULT
	));

	Accessibles.reserve(64);
}

monitor_impl::~monitor_impl()
{
	size_t nWatches = Watches.size();
	unwatch_all();
	
	backoff bo;
	while (NumActiveWatches)
		bo.pause();

	HANDLE hEvent = CreateEventA(0, FALSE, FALSE, nullptr);
	ResetEvent(hEvent);
	oVB_NOTHROW(DeleteTimerQueueTimer(hTimerQueue, hTimerQueueTimer, hEvent));

	HRESULT hr = WaitForSingleObject(hEvent, INFINITE);
	oAssertA(hr == WAIT_OBJECT_0, "failed waiting for DeleteTimerQueueTimer");

	oVB_NOTHROW(CloseHandle(hEvent));
	oVB_NOTHROW(DeleteTimerQueue(hTimerQueue));
}

void monitor_impl::on_event(const path_t& _Path, file_event _Event, double _Timestamp)
{
	// Files that are added or modified aren't necessarily usable, so queue
	// such files up for later analysis for accessibility.
	if (_Event == file_event::added || _Event == file_event::modified)
	{
		EVENT e;
		e.timestamp = _Timestamp;
		e.event = _Event;
		lock_guard<mutex> lock(EventsMutex);
		Events[_Path] = e;
	}

	if (OnEvent)
		OnEvent(_Event, _Path);
}

void monitor_impl::check_accessibility()
{
	lock_guard<mutex> lock(AccessibilityMutex);

	if (Events.empty())
		return;

	const double Timeout = Info.accessibility_timeout_ms / 1000.0;
	const double Now = timer::now();

	{
		lock_guard<mutex> lock_events(EventsMutex);
		for (auto it = std::begin(Events); it != std::end(Events); /* no increment */)
		{
			const path_t& p = it->first;
			const EVENT& e = it->second;

			if (is_accessible(p))
			{
				Accessibles.push_back(p);
				it = Events.erase(it);
			}

			else if ((e.timestamp + Timeout) > Now)
			{
				oTraceA("monitor: accessibility for %s timed out", p.c_str());
				it = Events.erase(it);
			}

			else
				++it;
		}
	}

	for (const auto& a : Accessibles)
		if (OnEvent)
			OnEvent(file_event::accessible, a);

	Accessibles.clear();
}

void monitor_impl::watch(const path_t& _Path, size_t _BufferSize, bool _Recursive)
{
	lock_guard<mutex> lock(WatchesMutex);

	for (auto it = std::begin(Watches); it != std::end(Watches); ++it)
		oCheck(!(*it)->watching(_Path), std::errc::operation_in_progress, "already watching %s", _Path.c_str());

	Watches.push_back(new watcher(this, _Path, _Recursive, _BufferSize));
	Watches.back()->watch_changes();

	NumActiveWatches++;
}

void monitor_impl::unwatch(const path_t& _Path)
{
	lock_guard<mutex> lock(WatchesMutex);
	for (auto it = std::begin(Watches); it != std::end(Watches); /* no inc */)
	{
		if ((*it)->watching(_Path))
		{
			(*it)->unwatch_changes();
			it = Watches.erase(it);
		}

		else
			++it;
	}
}

void monitor_impl::unwatch_all()
{
	lock_guard<mutex> lock(WatchesMutex);
	for (auto w : Watches)
		w->unwatch_changes();
	Watches.clear();
}

	} // namespace filesystem
}
