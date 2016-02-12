// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Approximation of the upcoming C++1x std::mutex objects.

#pragma once
#include <cassert>
#include <chrono>
#include <thread>

// To keep the main classes neat, collect all the platform-specific forward
// declaration here. This is done in this vague manner to avoid including 
// platform headers in this file.
namespace ouro {

	class mutex
	{
	public:
		mutex();
		~mutex();

		void lock();
		bool try_lock();
		void unlock();

		typedef void* native_handle_type;
		native_handle_type native_handle();

	protected:
		#if defined(_WIN32) || defined(_WIN64)
			void* footprint;
		#else
			#error unsupported platform (mutex)
		#endif
		#ifdef _DEBUG
			std::thread::id tid;
		#endif
	
	private:
		mutex(const mutex&); /* = delete */
		mutex& operator=(const mutex&); /* = delete */
	};

	class shared_mutex : public mutex
	{
	public:
		shared_mutex() {}

		void lock_shared();
		bool try_lock_shared();
		void unlock_shared();

	private:
		shared_mutex(const shared_mutex&); /* = delete */
		shared_mutex& operator=(const shared_mutex&); /* = delete */
	};

	class recursive_mutex
	{
	public:
		recursive_mutex();
		~recursive_mutex();

		void lock();
		bool try_lock();
		void unlock();

		typedef void* native_handle_type;
		native_handle_type native_handle();

	private:
		#ifdef _WIN64
			mutable unsigned long long footprint[5]; // RTL_CRITICAL_SECTION
		#elif defined(_WIN32)
			mutable unsigned int footprint[6];
		#else
			#error unsupported platform (recursive_mutex)
		#endif

		recursive_mutex(const recursive_mutex&); /* = delete */
		recursive_mutex& operator=(const recursive_mutex&); /* = delete */
	};

	class timed_mutex : public mutex
	{
	public:
		timed_mutex() {}
		~timed_mutex() {}

		template<typename Rep, typename Period>
		bool try_lock_for(std::chrono::duration<Rep,Period> const& relative_time)
		{
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(relative_time);
			Rep count = ms.count();
			if (Rep(count) != count)
				throw std::range_error("relative_time is too large");
			return try_lock_for(static_cast<unsigned int>(ms.count()));
		}

		template<typename Clock, typename Duration>
		bool try_lock_until(std::chrono::time_point<Clock,Duration> const& absolute_time)
		{
			auto duration = time_point_cast<std::chrono::high_resolution_clock::time_point>(absolute_time) - 
				std::chrono::high_resolution_clock::now();
			return try_lock_until(duration);
		}

	private:
		timed_mutex(timed_mutex const&); /* = delete */
		timed_mutex& operator=(timed_mutex const&); /* = delete */
		bool try_lock_for(unsigned int _TimeoutMS);
	};

	// Do not acquire ownership of the mutex.
	struct defer_lock_t {};
	// Try to acquire ownership of the mutex without blocking.
	struct try_to_lock_t {};
	// Assume the calling thread has already obtained mutex ownership and 
	// manage it.
	struct adopt_lock_t {};
	extern const adopt_lock_t adopt_lock;
	extern const defer_lock_t defer_lock;
	extern const try_to_lock_t try_to_lock;

	template <class Mutex> class lock_guard
	{
	public:
		typedef Mutex mutex_type;

		explicit lock_guard(mutex_type& mtx) : m(mtx) { m.lock(); }
		lock_guard(mutex_type& mtx, adopt_lock_t) : m(mtx) {}
		~lock_guard() { m.unlock(); }
	private:
		Mutex& m;
		lock_guard(const lock_guard&); /* = delete */
		lock_guard& operator=(const lock_guard&); /* = delete */
	};

	template<class Mutex> class shared_lock
	{
	public:
		typedef Mutex mutex_type;
		explicit shared_lock(mutex_type& mtx) : m(mtx) { m.lock_shared(); }
		shared_lock(mutex_type& mtx, adopt_lock_t) : m(mtx) {}
		~shared_lock() { m.unlock_shared(); }
	private:
		Mutex& m;
		shared_lock(const shared_lock&); /* = delete */
		shared_lock& operator=(const shared_lock&); /* = delete */
	};

#define assert_not_owner() \
	assert(mtx && "Must have valid mutex"); \
	assert((!mtx || (mtx && !owner)) && "Deadlock would occur")

	template <class Mutex> class unique_lock
	{
	public:
		typedef Mutex mutex_type;

		unique_lock()
			: mtx(nullptr)
			, owner(false)
		{}
		
		explicit unique_lock(mutex_type& mtx)
			: mtx(&mtx)
			, owner(false)
		{
			lock();
			owner = true;
		}

		unique_lock(mutex_type& mtx, adopt_lock_t)
			: mtx(&mtx)
			, owner(true)
		{}

		unique_lock(mutex_type& mtx, defer_lock_t)
			: mtx(&mtx)
			, owner(false)
		{}

		unique_lock(mutex_type& mtx, try_to_lock_t)
			: mtx(&mtx)
			, owner(mtx->try_lock())
		{}

		template<typename Clock,typename Duration>
		unique_lock(mutex_type& mtx, std::chrono::time_point<Clock,Duration> const& absolute_time)
			: mtx(&mtx)
			, owner(mtx->try_lock_until(absolute_time))
		{}

		template<typename Rep,typename Period>
		unique_lock(mutex_type& mtx, std::chrono::duration<Rep,Period> const& relative_time)
			: mtx(&mtx)
			, owner(mtx->try_lock_for(relative_time))
		{}

		~unique_lock()
		{
			if (owner)
				unlock();
		}

		unique_lock(unique_lock&& that)
			: owner(that.owner)
			, mtx(that.release())
		{}

		unique_lock& operator=(unique_lock&& that)
		{
			if (owner)
				unlock();

			owner = that.owner;
			mtx = that.release();
			return *this;
		}

		void swap(unique_lock&& that)
		{
			std::swap(mtx, that.mtx);
			std::swap(owner, that.owner);
		}

		void lock()
		{
			assert_not_owner();
			mtx->lock();
			owner = true;
		}

		bool try_lock()
		{
			assert_not_owner();
			owner = mtx->try_lock();
			return owner;
		}

		template<typename Rep, typename Period> bool try_lock_for(std::chrono::duration<Rep,Period> const& relative_time)
		{
			assert_not_owner();
			owner = mtx->try_lock_for(relative_time);
			return owner;
		}

		template<typename Clock, typename Duration> bool try_lock_until(std::chrono::time_point<Clock,Duration> const& absolute_time)
		{
			assert_not_owner();
			owner = mtx->try_lock_until(absolute_time);
			return owner;
		}

		void unlock()
		{
			assert(owner && "Cannot unlock a non-locked mutex (or a mutex whose lock is owned by another object)");
			if (mtx)
			{
				mtx->unlock();
				owner = false;
			}
		}

		/*explicit*/ operator bool() const { return owns_lock(); }
		bool owns_lock() const { return owner; }
		mutex_type* mutex() const { return mtx; }
		mutex_type* release() { mutex_type* copy = mtx; owner = false; mtx = nullptr; return copy; }

	private:
		mutex_type* mtx;
		bool owner;

		unique_lock(unique_lock const&); /* = delete */
		unique_lock& operator=(unique_lock const&); /* = delete */
	};

	class once_flag
	{
	public:
		/*constexpr*/ once_flag();

	private:
		#if defined(_WIN32) || defined(_WIN64)
			void* footprint;
		#else
			#error unsupported platform (once_flag)
		#endif
		once_flag(const once_flag&);
		once_flag& operator=(const once_flag&);
	};

	void call_once(once_flag& flag, const std::function<void>& fn);
}
