// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A cross-platform event built on condition_variable, which is basically how 
// the Window's event is implemented. This supports the "WaitMultiple" concept 
// by storing a 32-bit mask of bools, rather than just one bool like the Windows 
// event API suggests. Event flags can be defined then tested as appropriate. 
// The wait_any() APIs are more like WaitSingle. In more complex cases use 
// condition_variables directly. Using this implementation as a reference but 
// quite often an event is enough. NOTE: For a trivial bool-like event, just use 
// default parameters.

#pragma once
#include <condition_variable>
#include <mutex>

namespace ouro {

enum autoreset_t { autoreset };

class event
{
public:
	// Starts in the reset state. A call to set() sets the state and reset() 
	// resets the state explicitly. If the set() (and thus wake) and then reset()
	// needs to be atomic, use the autoreset constructor.
	event();

	// When set() is called, this will unblock all threads waiting on this event 
	// but then atomically leaves the event in the reset state.
	event(autoreset_t);

	// Sets the event mask by or'ing it with the existing mask, thus unblocking 
	// any threads waiting on this event.
	void set(int mask = 1);

	// Reset all set events in the specified mask. If autoreset is not specified 
	// no wait will block after a set() call until this is called.
	void reset(int mask = ~0);

	// Sleep the calling thread until all bits in the specified mask are set_mask().
	void wait(int mask = 1);

	// Sleep the calling thread until any bit in the specified mask is set_mask(). 
	// This will return the set_mask mask as it is when the wait is unblocked.
	int wait_any(int mask = 1);

	// Sleep the calling thread until the set_mask mask matches the one specified, or
	// absolute the timeout threshold is reached.
	template <typename Clock, typename Duration>
	bool wait_until(const std::chrono::time_point<Clock, Duration>& absolute_time, int mask = 1);

	// Sleep the calling thread until any bit in the set_mask mask is set, or the 
	// absolute timeout threshold is reached. This returns the mask as it is 
	// when the wait is unblocked.
	template <typename Clock, typename Duration>
	int wait_until_any(const std::chrono::time_point<Clock, Duration>& absolute_time, int mask = 1);

	// Sleep the calling thread until the set_mask mask matches the one specified, or
	// the relative timeout threshold is reached.
	template <typename Rep, typename Period>
	bool wait_for(const std::chrono::duration<Rep, Period>& relative_time, int mask = 1);

	// Sleep the calling thread until the set_mask mask matches the one specified, or
	// the relative timeout threshold is reached. This returns the mask as it is
	// when the wait is unblocked.
	template <typename Rep, typename Period>
	int wait_for_any(const std::chrono::duration<Rep, Period>& relative_time, int mask = 1);

	// Poll the state of the event mask and returns true if the masks match 
	// exactly. Use of this API is discouraged, but is sometimes useful for 
	// debug interrogatory API.
	bool is_set(int mask = 1) const;

	// Like is_set, but will return true if any bit in the mask is set
	bool is_any_set(int mask = 1) const;

private:
	std::mutex mtx;
	std::condition_variable cv;
	int set_mask;
	bool do_auto_reset;
};

inline event::event()
	: set_mask(0)
	, do_auto_reset(false)
{}

inline event::event(autoreset_t auto_reset)
	: set_mask(0)
	, do_auto_reset(true)
{}

inline void event::set(int mask)
{
	if (!is_set(mask))
	{
		if (do_auto_reset)
			cv.notify_one();
		else
		{
			mtx.lock();
			set_mask |= mask;
			mtx.unlock();
			cv.notify_all();
		}
	}
}

inline void event::reset(int mask)
{
	std::lock_guard<std::mutex> lock(mtx);
	set_mask &=~ mask;
}

inline void event::wait(int mask)
{
	std::unique_lock<std::mutex> lock(mtx);
	while (!is_set(mask))
		cv.wait(lock);
}

inline int event::wait_any(int mask)
{
	std::unique_lock<std::mutex> lock(mtx);
	while (!is_any_set(mask))
		cv.wait(lock);
	return set_mask;
}

template <typename Clock, typename Duration>
inline bool event::wait_until(
	const std::chrono::time_point<Clock, Duration>& absolute_time, int mask)
{
	std::unique_lock<std::mutex> lock(mtx);
	while (!is_set(mask))
		if (ouro::cv_status::timeout == cv.wait_until(lock, absolute_time))
			return false;
	return true;
}

template <typename Clock, typename Duration>
inline int event::wait_until_any(
	const std::chrono::time_point<Clock, Duration>& absolute_time, int mask)
{
	std::unique_lock<std::mutex> lock(mtx);
	while (!is_any_set(mask))
		if (ouro::cv_status::timeout == cv.wait_until(lock, absolute_time))
			return 0;
	return set_mask;
}

template <typename Rep, typename Period>
inline bool event::wait_for(const std::chrono::duration<Rep, Period>& relative_time, int mask)
{
	std::unique_lock<std::mutex> lock(mtx);
	while (!is_set(mask))
		if (std::cv_status::timeout == cv.wait_for(lock, relative_time))
			return false;
	return true;
}

template <typename Rep, typename Period>
inline int event::wait_for_any(const std::chrono::duration<Rep, Period>& relative_time, int mask)
{
	std::unique_lock<std::mutex> lock(mtx);
	while (!is_any_set(mask))
		if (ouro::cv_status::timeout == cv.wait_for(lock, relative_time))
			return 0;
	return set_mask;
}

inline bool event::is_set(int mask) const
{
	return (set_mask & mask) == mask;
}

inline bool event::is_any_set(int mask) const
{
	return !!(set_mask & mask);
}

}
