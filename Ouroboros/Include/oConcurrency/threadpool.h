// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A trivial thread pool that runs tasks on any thread at any time (no order 
// guarantees).

#pragma once
#include <oConcurrency/backoff.h>
#include <oConcurrency/countdown_latch.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ouro {

struct threadpool_default_traits
{
	// called once before worker thread main loop
	static void begin_thread(const char* thread_name) {}

	// called after each call to a dispatched user task()
	// (intended for RCU callouts)
	static void update_thread() {}

	// called once after worker thread main loop exits
	static void end_thread() {}
};

namespace detail { template<typename Traits, typename Alloc = std::allocator<std::function<void()>>> class task_group; }

template<typename Alloc>
class threadpool_base
{
public:
	typedef std::function<void()> task_type;
	typedef Alloc allocator_type;

	// Call construct_workers in most-derived ctor to initialize values.
	threadpool_base(const allocator_type& alloc = allocator_type());

	// Calls std::terminate() if join() wasn't explicitly called in client code,
	// the same as the behavior of std::thread.
	virtual ~threadpool_base();

	// Block until all workers are idle.
	void flush();

	// Returns true if the thread pool can still be joined.
	bool joinable() const;

	// Blocks until all workers are joined.
	void join();

protected:
	template<typename, typename> friend class detail::task_group;
	std::vector<std::thread> workers;
	std::deque<task_type, allocator_type> global_queue;
	std::mutex mtx;
	std::condition_variable work_available;
	size_t num_working;
	bool running;

	// Returns hardware_concurrency() if the specified number of workers is zero.
	size_t calc_num_workers(size_t num_workers_requested) const;

	// To ensure all members are constructed at time of worker instantiation, 
	// separate out a call to be called from the most-derived constructor.
	void construct_workers(const task_type& do_work, size_t num_workers = 0);

	threadpool_base(const threadpool_base&); /* = delete */
	const threadpool_base& operator=(const threadpool_base&); /* = delete */

	threadpool_base(threadpool_base&&); /* = delete */
	threadpool_base& operator=(threadpool_base&&); /* = delete */
};

template<typename Alloc>
size_t threadpool_base<Alloc>::calc_num_workers(size_t num_workers_requested) const
{
	return num_workers_requested ? num_workers_requested : std::thread::hardware_concurrency();
}

template<typename Alloc>
inline void threadpool_base<Alloc>::construct_workers(const task_type& do_work, size_t num_workers)
{
	num_working = calc_num_workers(num_workers);
	workers.resize(num_working);
	for (auto& w : workers)
		w = std::thread(do_work);
	flush(); // wait until all have settled and thus ensure all have initialized
}

template<typename Alloc>
inline threadpool_base<Alloc>::threadpool_base(const allocator_type& alloc)
	: global_queue(alloc)
	, num_working(0)
	, running(true)
{}

template<typename Alloc>
inline threadpool_base<Alloc>::~threadpool_base()
{
	if (joinable())
		std::terminate();
}

template<typename Alloc>
inline void threadpool_base<Alloc>::flush()
{
	ouro::backoff bo;
	while (running)
	{
		// global_queue.empty() is true before Task() is done, so don't use queue 
		// emptiness as an indicator of flushed-ness.
		if (global_queue.empty() && num_working == 0)
			break;
		bo.pause();
	}
}

template<typename Alloc>
inline bool threadpool_base<Alloc>::joinable() const
{
	return running && workers.front().joinable();
}

template<typename Alloc>
inline void threadpool_base<Alloc>::join()
{
	std::unique_lock<std::mutex> lock(mtx);
	running = false;
	work_available.notify_all();
	lock.unlock();
	for (auto& w : workers)
		w.join();
}

template<typename Traits, typename Alloc = std::allocator<std::function<void()>>>
class threadpool : public threadpool_base<Alloc>
{
public:
	typedef typename threadpool_base<Alloc>::task_type task_type;
	typedef typename threadpool_base<Alloc>::allocator_type allocator_type;

	// Pass 0 to allocate a worker thread for each hardware process found.
	threadpool(size_t num_workers = 0, const allocator_type& alloc = allocator_type());

	// The task will execute on any given worker thread. There is no order-of-
	// execution guarantee.
	void dispatch(const task_type& task);

private:
	void work();

	threadpool(const threadpool&); /* = delete */
	const threadpool& operator=(const threadpool&); /* = delete */

	threadpool(threadpool&&); /* = delete */
	threadpool& operator=(threadpool&&); /* = delete */
};

template<typename Traits, typename Alloc>
inline threadpool<Traits, Alloc>::threadpool(size_t num_workers, const allocator_type& alloc)
	: threadpool_base(alloc)
{
	construct_workers(std::bind(&threadpool::work, this), num_workers);
}

template<typename Traits, typename Alloc>
inline void threadpool<Traits, Alloc>::dispatch(const task_type& task)
{
	if (running)
	{
		std::unique_lock<std::mutex> lock(mtx);
		global_queue.push_back(task);
		if (num_working == 0)
			work_available.notify_one();
	}

	else
		oThrow(std::errc::invalid_argument, "dispatch called after join");
}

template<typename Traits, typename Alloc>
inline void threadpool<Traits, Alloc>::work()
{
	Traits::begin_thread("threadpool Worker");
	while (true)
	{
		std::unique_lock<std::mutex> lock(mtx);

		while (running && global_queue.empty())
		{
			num_working--;
			work_available.wait(lock);
			num_working++;
		}

		if (!running && global_queue.empty())
			break;

		task_type task = std::move(global_queue.front());
		global_queue.pop_front();
		lock.unlock();
		task();

		Traits::update_thread();
	}
	Traits::end_thread();
}

// task_group and parallel_for need to know the instance of a threadpool to work with, but 
// there should be only one threadpool in a system. oBase is not the place to solidify that
// policy, so provide the implementation here, but use it to implement a ouro-namespaced 
// task_group and parallel_for elsewhere.
	namespace detail {

template<typename Traits, typename Alloc>
class task_group
{
public:
	typedef Alloc allocator_type;
	typedef threadpool<Traits, Alloc> threadpool_type;
	task_group(threadpool_type& pool);
	~task_group();

	// Run a task as part of this task group
	void run(const std::function<void()>& task);

	// blocks until all tasks associated with this task group are finished. While
	// waiting, this work steals.
	void wait();

	// Cancels executing of all pending tasks in this group. If a task has already
	// executed, then it will be complete.
	void cancel();

	// Returns true if in the canceling state. The state is set on a call to cancel()
	// and is reset at the end of a wait.
	bool is_canceling();

private:
	threadpool_type& tp;
	ouro::countdown_latch latch;
	std::atomic_bool canceling;
};

template<typename Traits, typename Alloc>
task_group<Traits, Alloc>::task_group(threadpool_type& pool)
	: tp(pool)
	, latch(1)
{ canceling = false; }

template<typename Traits, typename Alloc>
task_group<Traits, Alloc>::~task_group()
{
	wait();
}

template<typename Traits, typename Alloc>
void task_group<Traits, Alloc>::run(const std::function<void()>& task)
{
	if (!canceling)
	{
		latch.reference();
		tp.dispatch([&,this,task] { if (!this->canceling) { task(); } latch.release(); });
	}
}

template<typename Traits, typename Alloc>
void task_group<Traits, Alloc>::wait()
{
	// should this do a num_working++ somewhere?

	latch.release();
	while (latch.outstanding())
	{
		std::unique_lock<std::mutex> lock(tp.mtx);

		if (!tp.running)
			oThrow(std::errc::invalid_argument, "threadpool shut down before task group could complete");

		if (tp.global_queue.empty())
		{
			lock.unlock();
			latch.wait();
		}

		else
		{
			auto task = std::move(tp.global_queue.front());
			tp.global_queue.pop_front();
			lock.unlock();
			task();
		}
	}

	latch.reset(1); // allow task group to be reused
	canceling.store(false);
}

template<typename Traits, typename Alloc>
void task_group<Traits, Alloc>::cancel()
{
	canceling.store(true);
}

template<typename Traits, typename Alloc>
bool task_group<Traits, Alloc>::is_canceling()
{
	return canceling;
}

template<typename Traits, typename Alloc>
inline void thread_local_parallel_for(task_group<Traits, Alloc>& group, size_t begin, size_t end, const std::function<void(size_t index)>& task)
{
	for (; begin < end; begin++)
		group.run(std::bind(task, begin));
}

template<size_t WorkChunkSize /* = 16*/, typename Traits, typename Alloc>
inline void parallel_for(threadpool<Traits, Alloc>& pool, size_t begin, size_t end, const std::function<void(size_t index)>& task)
{
	task_group<Traits, Alloc> g(pool);
	const size_t kNumSteps = (end - begin) / WorkChunkSize;
	for (size_t i = 0; i < kNumSteps; i++, begin += WorkChunkSize)
		g.run(std::bind(thread_local_parallel_for<Traits, Alloc>, std::ref(g), begin, begin + WorkChunkSize, task));
	if (begin < end)
		g.run(std::bind(thread_local_parallel_for<Traits, Alloc>, std::ref(g), begin, end, task));
	g.wait();
}

	}
}
