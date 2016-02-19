// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/windows/win_iocp.h>
#include <oConcurrency/backoff.h>
#include <oMemory/allocate.h>
#include <oMemory/concurrent_object_pool.h>
#include <oSystem/debugger.h>
#include <oSystem/process_heap.h>
#include <oSystem/reporting.h>
#include <oSystem/windows/win_error.h>
#include <thread>
#include <vector>

using namespace std;

namespace ouro {
	namespace windows {

#if _DEBUG
class local_timeout
{
	// For those times a loop should only continue for a limited time, favor using
	// this class as it is more self-documenting and more centralized than just
	// doing the math directly.
	double end_;
public:
	local_timeout(double timeout) { reset(timeout); }
	local_timeout(uint32_t timeout_ms) { reset(timeout_ms / 1000.0); }
	inline void reset(double timeout) { end_ = timer::now() + timeout; }
	inline void reset(uint32_t timeout_ms) { reset(timeout_ms / 1000.0); }
	inline bool timed_out() const { return timer::now() >= end_; }
};
#endif

namespace op
{	enum value {

	shutdown = 1,
	completion,
	
	// This is used for the numbytes field instead of the key so that both the 
	// key and overlapped pointer-sized parameters can be used.
	post = ~0u,

};}

struct iocp_overlapped : public OVERLAPPED
{
	HANDLE handle;
	iocp::completion_t completion;
	void* context;
};

class iocp_threadpool
{
public:
	static unsigned int concurrency();

	static iocp_threadpool& singleton();
	static void* find_instance();

	// Waits for all work to be completed
	void wait() { wait_for(~0u); }
	bool wait_for(unsigned int _TimeoutMS);
	bool joinable() const;
	void join();

	OVERLAPPED* associate(HANDLE _Handle, iocp::completion_t _Completion, void* _pContext);
	void disassociate(OVERLAPPED* _pOverlapped);
	void post_completion(OVERLAPPED* _pOverlapped);
	void post(iocp::completion_t _Completion, void* _pContext);
private:
	iocp_threadpool() : hIoPort(INVALID_HANDLE_VALUE), NumRunningThreads(0), NumAssociations(0) {}
	iocp_threadpool(size_t _OverlappedCapacity, size_t _NumWorkers = 0);
	~iocp_threadpool();
	iocp_threadpool(iocp_threadpool&& _That) { operator=(move(_That)); }
	iocp_threadpool& operator=(iocp_threadpool&& _That);

	void work();

	HANDLE hIoPort;
	vector<thread> Workers;
	atomic<size_t> NumRunningThreads;
	size_t NumAssociations;

	concurrent_object_pool<iocp_overlapped> pool;

	iocp_threadpool(const iocp_threadpool&); /* = delete; */
	const iocp_threadpool& operator=(const iocp_threadpool&); /* = delete; */
};

unsigned int iocp_threadpool::concurrency()
{
	return thread::hardware_concurrency();
}

void iocp_threadpool::work()
{
	debugger::thread_name("iocp worker");
	NumRunningThreads++;
	while (true)
	{
		bool CallCompletion = false;
		DWORD nBytes = 0;
		ULONG_PTR key = 0;
		iocp_overlapped* ol = nullptr;
		if (GetQueuedCompletionStatus(hIoPort, &nBytes, &key, (OVERLAPPED**)&ol, INFINITE))
		{
			if (op::post == nBytes)
			{
				try
				{
					iocp::completion_t complete = (iocp::completion_t)key;
					if (complete)
						complete(ol, 0);
				}

				catch (std::exception& e)
				{
					e;
					oTraceA("iocp post failed: %s", e.what());
				}
			}

			else if (op::shutdown == key)
				break;
			
			else if (op::completion == key)
				CallCompletion = true;
			else
				oThrow(std::errc::operation_not_supported, "CompletionKey %p not supported", key);
		}
		else if (ol)
				CallCompletion = true;

		if (CallCompletion)
		{
			try
			{
				if (ol->completion)
					ol->completion(ol->context, nBytes);
			}

			catch (std::exception& e)
			{
				e;
				oTraceA("iocp completion failed: %s", e.what());
			}
		}
	}
	NumRunningThreads--;
}

iocp_threadpool& iocp_threadpool::singleton()
{
	static iocp_threadpool* sInstance = nullptr;
	if (!sInstance)
	{
		process_heap::find_or_allocate(
			"iocp"
			, process_heap::per_process
			, process_heap::garbage_collected
			, [=](void* _pMemory) { new (_pMemory) iocp_threadpool(128); }
			, [=](void* _pMemory) { ((iocp_threadpool*)_pMemory)->~iocp_threadpool(); }
			, &sInstance);

			// This is a workaround for some stuff going on in libc. Basically when
			// these threads end, they lock a libc mutex which can be created for the
			// first time, so it does some static stuff, including thinking about 
			// atexit. This dtor executes during atexit, so basically there's a 
			// deadlock. Tickle that mutex by destroying threads here, but then bring
			// it all back up.
			sInstance->~iocp_threadpool();
			new (sInstance) iocp_threadpool(128);
	}
	return *sInstance;
}

void* iocp_threadpool::find_instance()
{
	void* pInstance = nullptr;
	process_heap::find("iocp", process_heap::per_process, &pInstance);
	return pInstance;
}

iocp_threadpool::iocp_threadpool(size_t _OverlappedCapacity, size_t _NumWorkers)
	: hIoPort(nullptr)
	, NumRunningThreads(0)
	, NumAssociations(0)
{
	auto req = pool.calc_size((uint32_t)_OverlappedCapacity);
	void* mem = default_allocate(req, "iocp_threadpool", memory_alignment::cacheline);

	pool.initialize(mem, req);
	
	reporting::ensure_initialized();

	const size_t NumWorkers = _NumWorkers ? _NumWorkers : thread::hardware_concurrency();
	hIoPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, static_cast<DWORD>(NumWorkers));
	oVB(hIoPort);

	Workers.resize(NumWorkers);
	auto worker = bind(&iocp_threadpool::work, this);
	NumRunningThreads = 0;
	for (auto& w : Workers)
		w = move(thread(worker));

	backoff bo;
	while (NumRunningThreads != Workers.size())
		bo.pause();
}

iocp_threadpool::~iocp_threadpool()
{
	// there was a race condition where the leak tracker was inside crt malloc, which was holding a mutex, then
	// iocp threads would do something that triggered an init of a static mutex, which tried to load concrt which
	// then would grab the crt mutex and deadlock on malloc's mutex. To get around this, all iocp threads should
	// be dead before static deinit - but that's app-specific. I just fixed a bug in process_heap destruction order
	// that may make the issue go away, so join here for now and see if the race condition pops back up.
	// If it doesn't, look for exposed uses of join (filesystem) and remove them to simplify user requirements.
	if (joinable())
		join();

	if (joinable())
		std::terminate();

	pool.deinitialize();
}

iocp_threadpool& iocp_threadpool::operator=(iocp_threadpool&& _That)
{
	if (this != &_That)
	{
		hIoPort = _That.hIoPort; _That.hIoPort = INVALID_HANDLE_VALUE;
		Workers = move(_That.Workers);
		NumRunningThreads.store(_That.NumRunningThreads); _That.NumRunningThreads = 0;
		NumAssociations = _That.NumAssociations; _That.NumAssociations = 0;
		pool = move(_That.pool);
	}
	return *this;
}

bool iocp_threadpool::wait_for(unsigned int _TimeoutMS)
{
	backoff bo;

	unsigned int start = timer::now_msi();

	#ifdef _DEBUG
		local_timeout to(5.0);
	#endif

	while (NumAssociations > 0)
	{ 
		if (_TimeoutMS != ~0u && timer::now_msi() >= (start + _TimeoutMS))
			return false;

		bo.pause();

		#ifdef _DEBUG
			if (to.timed_out())
			{
				oTrace("Waiting for %u outstanding iocp associations to finish...", NumAssociations);
				to.reset(5.0);
			}
		#endif
	}

	return true;
}

bool iocp_threadpool::joinable() const
{
	return INVALID_HANDLE_VALUE != hIoPort;
}

void iocp_threadpool::join()
{
	oCheck(joinable(), std::errc::invalid_argument, "");
	oCheck(wait_for(20000), std::errc::timed_out, "timed out waiting for iocp completion");

	for (auto& w : Workers)
		PostQueuedCompletionStatus(hIoPort, 0, op::shutdown, nullptr);

	for (auto& w : Workers)
		if (w.joinable())
			w.join();

	if (INVALID_HANDLE_VALUE != hIoPort)
	{
		CloseHandle(hIoPort);
		hIoPort = INVALID_HANDLE_VALUE;
	}
}

OVERLAPPED* iocp_threadpool::associate(HANDLE _Handle, iocp::completion_t _Completion, void* _pContext)
{
	NumAssociations++;
	iocp_overlapped* ol = pool.create();
	if (ol)
	{
		if (hIoPort != CreateIoCompletionPort(_Handle, hIoPort, op::completion, static_cast<DWORD>(Workers.size())))
		{
			disassociate(ol);
			oVB(false);
		}

		memset(ol, 0, sizeof(OVERLAPPED));
		ol->handle = _Handle;
		ol->completion = _Completion;
		ol->context = _pContext;
	}

	else
		NumAssociations--;

	return ol;
}

void iocp_threadpool::disassociate(OVERLAPPED* _pOverlapped)
{
	pool.destroy(static_cast<iocp_overlapped*>(_pOverlapped));
	NumAssociations--;
}

void iocp_threadpool::post_completion(OVERLAPPED* _pOverlapped)
{
	PostQueuedCompletionStatus(hIoPort, 0, op::completion, _pOverlapped);
}

void iocp_threadpool::post(iocp::completion_t _Completion, void* _pContext)
{
	PostQueuedCompletionStatus(hIoPort, (DWORD)op::post, (ULONG_PTR)_Completion, (OVERLAPPED*)_pContext);
}

namespace iocp {

unsigned int concurrency()
{
	return iocp_threadpool::concurrency();
}

void ensure_initialized()
{
	iocp_threadpool::singleton();
}

OVERLAPPED* associate(HANDLE _Handle, completion_t _Completion, void* _pContext)
{
	return iocp_threadpool::singleton().associate(_Handle, _Completion, _pContext);
}

void disassociate(OVERLAPPED* _pOverlapped)
{
	iocp_threadpool::singleton().disassociate(_pOverlapped);
}

void post_completion(OVERLAPPED* _pOverlapped)
{
	iocp_threadpool::singleton().disassociate(_pOverlapped);
}

void post(completion_t _Completion, void* _pContext)
{
	iocp_threadpool::singleton().post(_Completion, _pContext);
}

void wait()
{
	iocp_threadpool::singleton().wait();
}

bool wait_for(unsigned int _TimeoutMS)
{
	return iocp_threadpool::singleton().wait_for(_TimeoutMS);
}

bool joinable()
{
	return iocp_threadpool::singleton().joinable();
}

void join()
{
	return iocp_threadpool::singleton().join();
}

		} // namespace iocp
	}
}
