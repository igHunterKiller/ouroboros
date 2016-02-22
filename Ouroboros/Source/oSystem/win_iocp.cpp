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

namespace ouro { namespace windows {

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
	iocp::completion_fn completion;
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
	bool wait_for(unsigned int timeout_ms);
	bool joinable() const;
	void join();

	OVERLAPPED* associate(HANDLE handle, iocp::completion_fn completion, void* context);
	void disassociate(OVERLAPPED* overlapped);
	void post_completion(OVERLAPPED* overlapped);
	void post(iocp::completion_fn completion, void* context);
private:
	iocp_threadpool() : hioport_(INVALID_HANDLE_VALUE), num_running_threads_(0), num_associations_(0) {}
	iocp_threadpool(size_t _OverlappedCapacity, size_t _NumWorkers = 0);
	~iocp_threadpool();
	iocp_threadpool(iocp_threadpool&& _That) { operator=(move(_That)); }
	iocp_threadpool& operator=(iocp_threadpool&& _That);

	void work();

	HANDLE         hioport_;
	vector<thread> workers_;
	atomic<size_t> num_running_threads_;
	size_t         num_associations_;

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
	num_running_threads_++;
	while (true)
	{
		bool call_completion = false;
		DWORD nbytes = 0;
		ULONG_PTR key = 0;
		iocp_overlapped* ol = nullptr;
		if (GetQueuedCompletionStatus(hioport_, &nbytes, &key, (OVERLAPPED**)&ol, INFINITE))
		{
			if (op::post == nbytes)
			{
				try
				{
					iocp::completion_fn complete = (iocp::completion_fn)key;
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
				call_completion = true;
			else
				oThrow(std::errc::operation_not_supported, "CompletionKey %p not supported", key);
		}
		else if (ol)
				call_completion = true;

		if (call_completion)
		{
			try
			{
				if (ol->completion)
					ol->completion(ol->context, nbytes);
			}

			catch (std::exception& e)
			{
				e;
				oTraceA("iocp completion failed: %s", e.what());
			}
		}
	}
	num_running_threads_--;
}

iocp_threadpool& iocp_threadpool::singleton()
{
	static iocp_threadpool* s_instance = nullptr;
	if (!s_instance)
	{
		process_heap::find_or_allocate(
			"iocp"
			, process_heap::per_process
			, process_heap::garbage_collected
			, [=](void* _pMemory) { new (_pMemory) iocp_threadpool(128); }
			, [=](void* _pMemory) { ((iocp_threadpool*)_pMemory)->~iocp_threadpool(); }
			, &s_instance);

			// This is a workaround for some stuff going on in libc. Basically when
			// these threads end, they lock a libc mutex which can be created for the
			// first time, so it does some static stuff, including thinking about 
			// atexit. This dtor executes during atexit, so basically there's a 
			// deadlock. Tickle that mutex by destroying threads here, but then bring
			// it all back up.
			s_instance->~iocp_threadpool();
			new (s_instance) iocp_threadpool(128);
	}
	return *s_instance;
}

void* iocp_threadpool::find_instance()
{
	void* pInstance = nullptr;
	process_heap::find("iocp", process_heap::per_process, &pInstance);
	return pInstance;
}

iocp_threadpool::iocp_threadpool(size_t _OverlappedCapacity, size_t _NumWorkers)
	: hioport_(nullptr)
	, num_running_threads_(0)
	, num_associations_(0)
{
	auto req = pool.calc_size((uint32_t)_OverlappedCapacity);
	void* mem = default_allocate(req, "iocp_threadpool", memory_alignment::cacheline);

	pool.initialize(mem, req);
	
	reporting::ensure_initialized();

	const size_t NumWorkers = _NumWorkers ? _NumWorkers : thread::hardware_concurrency();
	hioport_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, static_cast<DWORD>(NumWorkers));
	oVB(hioport_);

	workers_.resize(NumWorkers);
	auto worker = bind(&iocp_threadpool::work, this);
	num_running_threads_ = 0;
	for (auto& w : workers_)
		w = move(thread(worker));

	backoff bo;
	while (num_running_threads_ != workers_.size())
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
		hioport_ = _That.hioport_; _That.hioport_ = INVALID_HANDLE_VALUE;
		workers_ = move(_That.workers_);
		num_running_threads_.store(_That.num_running_threads_); _That.num_running_threads_ = 0;
		num_associations_ = _That.num_associations_; _That.num_associations_ = 0;
		pool = move(_That.pool);
	}
	return *this;
}

bool iocp_threadpool::wait_for(unsigned int timeout_ms)
{
	backoff bo;

	unsigned int start = timer::now_msi();

	#ifdef _DEBUG
		local_timeout to(5.0);
	#endif

	while (num_associations_ > 0)
	{ 
		if (timeout_ms != ~0u && timer::now_msi() >= (start + timeout_ms))
			return false;

		bo.pause();

		#ifdef _DEBUG
			if (to.timed_out())
			{
				oTrace("Waiting for %u outstanding iocp associations to finish...", num_associations_);
				to.reset(5.0);
			}
		#endif
	}

	return true;
}

bool iocp_threadpool::joinable() const
{
	return INVALID_HANDLE_VALUE != hioport_;
}

void iocp_threadpool::join()
{
	oCheck(joinable(), std::errc::invalid_argument, "");
	oCheck(wait_for(20000), std::errc::timed_out, "timed out waiting for iocp completion");

	for (auto& w : workers_)
		PostQueuedCompletionStatus(hioport_, 0, op::shutdown, nullptr);

	for (auto& w : workers_)
		if (w.joinable())
			w.join();

	if (INVALID_HANDLE_VALUE != hioport_)
	{
		CloseHandle(hioport_);
		hioport_ = INVALID_HANDLE_VALUE;
	}
}

OVERLAPPED* iocp_threadpool::associate(HANDLE handle, iocp::completion_fn completion, void* context)
{
	num_associations_++;
	iocp_overlapped* ol = pool.create();
	if (ol)
	{
		if (hioport_ != CreateIoCompletionPort(handle, hioport_, op::completion, static_cast<DWORD>(workers_.size())))
		{
			disassociate(ol);
			oVB(false);
		}

		memset(ol, 0, sizeof(OVERLAPPED));
		ol->handle = handle;
		ol->completion = completion;
		ol->context = context;
	}

	else
		num_associations_--;

	return ol;
}

void iocp_threadpool::disassociate(OVERLAPPED* overlapped)
{
	pool.destroy(static_cast<iocp_overlapped*>(overlapped));
	num_associations_--;
}

void iocp_threadpool::post_completion(OVERLAPPED* overlapped)
{
	PostQueuedCompletionStatus(hioport_, 0, op::completion, overlapped);
}

void iocp_threadpool::post(iocp::completion_fn completion, void* context)
{
	PostQueuedCompletionStatus(hioport_, (DWORD)op::post, (ULONG_PTR)completion, (OVERLAPPED*)context);
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

OVERLAPPED* associate(HANDLE handle, completion_fn completion, void* context)
{
	return iocp_threadpool::singleton().associate(handle, completion, context);
}

void disassociate(OVERLAPPED* overlapped)
{
	iocp_threadpool::singleton().disassociate(overlapped);
}

void post_completion(OVERLAPPED* overlapped)
{
	iocp_threadpool::singleton().disassociate(overlapped);
}

void post(completion_fn completion, void* context)
{
	iocp_threadpool::singleton().post(completion, context);
}

void wait()
{
	iocp_threadpool::singleton().wait();
}

bool wait_for(unsigned int timeout_ms)
{
	return iocp_threadpool::singleton().wait_for(timeout_ms);
}

bool joinable()
{
	return iocp_threadpool::singleton().joinable();
}

void join()
{
	return iocp_threadpool::singleton().join();
}

}}}
