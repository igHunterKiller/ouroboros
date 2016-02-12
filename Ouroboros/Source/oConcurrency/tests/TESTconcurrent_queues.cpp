// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/finally.h>
#include <oConcurrency/concurrency.h>
#include <oConcurrency/concurrent_queue.h>
#include <oConcurrency/concurrent_queue_opt.h>
#include <oConcurrency/threadpool.h>
#include <oConcurrency/event.h>
#include <atomic>
#include <thread>
#include <vector>

using namespace ouro;

struct test_buffer
{
	enum Dummy_t { Dummy };

	test_buffer(Dummy_t _Dummy) : Pointer(nullptr), pNumDeleted(nullptr) {}
	test_buffer(size_t _Size, size_t* _pNumDeleted) : Pointer(new char[_Size]), pNumDeleted(_pNumDeleted) {}
	~test_buffer() { (*pNumDeleted)++; delete [] Pointer; }
	bool operator==(const test_buffer& _That) const { return Pointer == _That.Pointer; }

	char* Pointer;
	size_t* pNumDeleted;
};

template<typename T, typename QueueT>
static void test_queue_basics(unit_test::services& services, const char* queue_name)
{
	QueueT q;

	static const T kNumPushes = 10;

	for (T i = 0; i < kNumPushes; i++)
		q.push(i);

	oCHECK(!q.empty(), "%s reports empty when it's not", queue_name);
	oCHECK(q.size() == kNumPushes, "%s reports size %u, expected %u", queue_name, q.size(), kNumPushes);

	int test = -1;
	for (T i = 0; i < (kNumPushes-1); i++)
		q.pop(test);

	oCHECK(!q.empty(), "%s should have 1 element, but reports as empty", queue_name);
	oCHECK(q.size() == 1, "%s reports size %u, expected 1", queue_name, q.size());

	q.clear();
	oCHECK(q.empty(), "%s cleared, but reports non-empty when it's empty", queue_name);
	oCHECK(q.size() == 0, "%s reports size %u, expected %u", queue_name, q.size(), 0);

	for (T i = 0; i < kNumPushes; i++)
		q.push(i);

	test = -1;
	if (is_fifo<QueueT>::value)
	{
		for (T i = 0; i < kNumPushes; i++)
		{
			oCHECK(q.try_pop(test), "%s TryPop failed", queue_name);
			oCHECK(test == i, "%s value from TryPop failed", queue_name);
		}
	}

	else
	{
		for (T i = kNumPushes-1; i >= 0; i--)
		{
			oCHECK(q.try_pop(test), "%s TryPop failed", queue_name);
			oCHECK(test == i, "%s value from TryPop failed", queue_name);
		}
	}

	oCHECK(!q.try_pop(test), "%s TryPop on an empty list succeeded (it shouldn't)", queue_name);
	oCHECK(q.empty(), "%s not empty", queue_name);
	oCHECK(q.size() == 0, "%s should be empty and thus report size 0, got size %u", queue_name, q.size());
}

template<typename T, typename QueueT>
static void push_task(QueueT* _pQueue, size_t _NumPushes)
{
	for (size_t i = 0; i < _NumPushes; i++)
		_pQueue->push(1);
}

template<typename T, typename QueueT>
static void pop_task(QueueT* _pQueue, size_t _NumPops)
{
	T e;
	for (size_t i = 0; i < _NumPops; i++)
		_pQueue->pop(e);
}

template<typename T, typename QueueT>
static void push_and_pop_task(QueueT* _pQueue, size_t _NumPushPops, size_t _NumIterations)
{
	while (--_NumIterations)
	{
		for (size_t i = 0; i < _NumPushPops; i++)
			_pQueue->push(1);

		int j = 0;
		for (size_t i = 0; i < _NumPushPops; i++)
			_pQueue->pop(j);
	}

	//oTRACE("TESTQueues_PushAndPop finished (0x%x)", asuint(this_thread::get_id()));
}

template<typename T, typename QueueT>
static void push_and_pop_task_performance(QueueT* _pQueue, size_t _NumPushPops, size_t _NumIterations, event* _pStart)
{
	_pStart->wait();
	push_and_pop_task<T, QueueT>(_pQueue, _NumPushPops, _NumIterations);
	oTRACE("Done thread.");
}

template<typename T, typename QueueT>
static void test_concurrent_pushes(unit_test::services& services, const char* queue_name)
{
	QueueT q;
	oFinally { q.clear(); };

	const size_t NumPushes = 1000;
	size_t ExpectedSize = 0;
	oCHECK(q.size() == ExpectedSize, "Empty queue not reporting 0 elements");

	// Scope to ensure queue is cleaned up AFTER all threads.
	{
		std::vector<std::thread> threadArray(std::thread::hardware_concurrency() + 5); // throw in some contention
		ExpectedSize = NumPushes * threadArray.size();

		for (size_t i = 0; i < threadArray.size(); i++)
			threadArray[i] = std::thread(push_task<T, QueueT>, &q, NumPushes);

		for (size_t i = 0; i < threadArray.size(); i++)
			threadArray[i].join();
	}

	size_t ActualSize = q.size();
	oCHECK(ActualSize == ExpectedSize, "Queue doesn't have as many items as it should");
}

template<typename T, typename QueueT>
static void test_concurrent_pops(unit_test::services& services, const char* queue_name)
{
	QueueT q;
	try
	{
		size_t ActualSize = q.size();
		oCHECK(ActualSize == 0, "Queue should have initialized to zero items, but has %u", ActualSize);

		// Scope to ensure queue is cleaned up AFTER all threads.
		const size_t NumPops = 100;
		{
			std::vector<std::thread> threadArray(std::thread::hardware_concurrency() + 5); // throw in some contention
			size_t InitialSize = T(NumPops * threadArray.size());
			for (T i = 0; i < T(InitialSize); i++)
				q.push(i);

			ActualSize = q.size();
			oCHECK(ActualSize == InitialSize, "size (%u) != #pushes (%u)", ActualSize, InitialSize);

			for (size_t i = 0; i < threadArray.size(); i++)
				threadArray[i] = std::thread(pop_task<T, QueueT>, &q, NumPops);

			for (size_t i = 0; i < threadArray.size(); i++)
				threadArray[i].join();
		}

		oCHECK(q.empty(), "Queue should be empty, but isn't");
		ActualSize = q.size();
		oCHECK(ActualSize == 0, "Queue should have zero items, but has %u", ActualSize);
	}

	catch(std::exception&)
	{
		q.clear();
	}
}

template<typename T, typename QueueT>
static void test_concurrency(unit_test::services& services, const char* queue_name)
{
	QueueT q;

	// Scope to ensure queue is cleaned up AFTER all threads.
	{
		std::vector<std::thread> threadArray(std::thread::hardware_concurrency() + 5); // throw in some contention

		for (size_t i = 0; i < threadArray.size(); i++)
			threadArray[i] = std::thread(push_and_pop_task<T, QueueT>, &q, 1000, 100);

		for (size_t i = 0; i < threadArray.size(); i++)
			threadArray[i].join();
	}

	oCHECK(q.empty(), "Queue is not empty after all threads have been joined");
	oCHECK(q.size() == 0, "Queue is empty, but size is non-zero (%u)", q.size());
}

template<typename T, typename QueueT>
static double test_performance(unit_test::services& services, const char* queue_name)
{
	QueueT q;
	event Start;
	double time = 0.0;

	static const size_t NumPushPops = 5000;
	static const size_t NumIterations = 1000;

	// Scope to ensure queue is cleaned up AFTER all threads.
	{
		std::vector<thread> threadArray(thread::hardware_concurrency());
		std::vector<double> Times(threadArray.size(), 0.0);

		for (size_t i = 0; i < threadArray.size(); i++)
			threadArray[i] = std::move(thread(push_and_pop_task_performance<T, QueueT>, &q, NumPushPops, NumIterations, &Start));

		ouro::finally JoinThreads([&]
		{
			for (size_t i = 0; i < threadArray.size(); i++)
				threadArray[i].join();
		});

		// Get all thread setup out of the way
		this_thread::sleep_for(chrono::milliseconds(500));
		Start.set();

		local_timer t;

		//JoinThreads();
		for (size_t i = 0; i < threadArray.size(); i++)
			threadArray[i].join();
		oTRACE("Done main.");

		time = t.seconds();
	
		//this_thread::sleep_for(chrono::milliseconds(500));
	}

	return time;
}

bool test_bufferCreate(size_t _Size, size_t* _pNumDeleted, test_buffer** _ppBuffer)
{
	*_ppBuffer = new test_buffer(_Size, _pNumDeleted);
	return true;
}

template<typename QueueT>
static void test_nontrivial_contents(unit_test::services& services, const char* queue_name)
{
	QueueT q;

	static const size_t kNumBuffers = 3;
	static const size_t kBufferSize = 1024;

	size_t numDeleted = 0;
	
	// Create a list of buffers and push them onto the queue, then lose the refs
	// of the original buffers.
	{
		std::shared_ptr<test_buffer> Buffers[kNumBuffers];
		for (size_t i = 0; i < kNumBuffers; i++)
		{
			Buffers[i] = std::make_shared<test_buffer>(kBufferSize, &numDeleted);
			memset(Buffers[i]->Pointer, 123, kBufferSize);
			q.push(Buffers[i]);
		}
	}
	oCHECK(numDeleted == 0, "Test buffers were deleted when the queue should be holding a reference");

	std::shared_ptr<test_buffer> b;
	while (q.try_pop(b))
	{
		const char* p = b->Pointer;
		for (size_t i = 0; i < kBufferSize; i++)
			oCHECK(*p++ == 123, "Buffer mismatch on byte %u", i);
	}

	b = nullptr;

	oCHECK(q.empty() && numDeleted == kNumBuffers, "Queue is retaining a reference to an element though it's supposed to be empty.");
}

template<typename IntQueueT, typename test_bufferRefQueueT>
void TestQueueT(unit_test::services& services, const char* queue_name)
{
	test_queue_basics<int, IntQueueT>(services, queue_name);
	test_concurrent_pushes<int, IntQueueT>(services, queue_name);
	test_concurrent_pops<int, IntQueueT>(services, queue_name);
	test_concurrency<int, IntQueueT>(services, queue_name);
	test_nontrivial_contents<test_bufferRefQueueT>(services, queue_name);

	// Enable this for some performance reporting (assuming it still passes the
	// above tests!) NOTE: for a fair test, use a custom block allocator for 
	// ouro implementations.
	#if 0
		double time = test_performance<int, IntQueueT>(services, queue_name);
		services.report("%s perf test: %.03fs", queue_name, time);
	#endif
}

#define oTEST_QUEUET(_QueueType) TestQueueT<_QueueType<int>, _QueueType<std::shared_ptr<test_buffer>>>(srv, #_QueueType)

oTEST(oConcurrency_concurrent_queue)
{
	oTEST_QUEUET(concurrent_queue);
}

oTEST(oConcurrency_concurrent_queue_opt)
{
	oTEST_QUEUET(concurrent_queue_opt);
}

static const int kNumTasks = 50000;
static const int kPoppedFlag = 0x80000000;
static const int kStolenFlag = 0x40000000;
static const int kTouchedMask = 0xc0000000;
static const int kValueMask = 0x3fffffff;

typedef threadpool_default_traits thread_traits_t;

#if 0
// Prove concurrent_queue is dropin for concurrent_queue
}}
#include <concurrent_queue.h>
using namespace ouro;
template<typename T> class concurrent_queue_concrt : private ::Concurrency::concurrent_queue<T>, public concurrent_queue_base<T, concurrent_queue_concrt<T>>
{
	typedef Concurrency::concurrent_queue<T> queue_t;
public:
	oDEFINE_CONCURRENT_QUEUE_TYPE(T, queue_t::size_type);

	concurrent_queue_concrt() {}
	void push(const_reference _Element) { oThreadsafe(this)->queue_t::push(_Element); }
	bool try_pop(reference _Element) { return oThreadsafe(this)->queue_t::try_pop(_Element); }
	void clear() { return concurrent_queue_base::clear(); }
	bool empty() const { return oThreadsafe(this)->queue_t::empty(); }
	size_type size() const { return queue_t::unsafe_size(); }
};

oTEST(oConcurrency_concurrent_queue_concrt)
{
	oTEST_QUEUET(concurrent_queue_concrt);
}

#else
oTEST(oConcurrency_concurrent_queue_concrt)
{
	throw unit_test::skip_test("concurrent_queue_concrt test not enabled");
}
#endif

#if 0
}}
#include <tbb/concurrent_queue.h>
using namespace ouro;
template<typename T> class concurrent_queue_tbb : private tbb::concurrent_queue<T>, public concurrent_queue_base<T, concurrent_queue_tbb<T>>
{
	typedef tbb::concurrent_queue<T> queue_t;
public:
	oDEFINE_CONCURRENT_QUEUE_TYPE(T, queue_t::size_type);

	concurrent_queue_tbb() {}
	void push(const_reference _Element) { oThreadsafe(this)->queue_t::push(_Element); }
	bool try_pop(reference _Element) { return oThreadsafe(this)->queue_t::try_pop(_Element); }
	void clear() { return concurrent_queue_base::clear(); }
	bool empty() const { return oThreadsafe(this)->queue_t::empty(); }
	size_type size() const { return queue_t::unsafe_size(); }
};

oTEST(oConcurrency_concurrent_queue_tbb)
{
	oTEST_QUEUET(concurrent_queue_tbb);
}
#else
oTEST(oConcurrency_concurrent_queue_tbb)
{
	throw unit_test::skip_test("concurrent_queue_tbb test not enabled");
}
#endif
