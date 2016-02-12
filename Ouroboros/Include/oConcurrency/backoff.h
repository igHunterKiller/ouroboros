// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A smarter spin lock useful as a prelude to a mutex lock or other act that 
// will remove a thread from its timeslice. (from TBB source)

#pragma once
#include <thread>

namespace ouro {

class backoff
{
public:
	backoff();

	// A spin that degrades (spins longer) with each call until it makes more 
	// sense to yield.
	void pause();

	// A spin that degrades (spins longer) with each call (returning true) until 
	// it would yield, then it returns false without yielding.
	bool try_pause();

	// Resets the degradation of the spin loop.
	void reset();

private:
	static const size_t threshold = 16;
	size_t spin_count;
	void spin(size_t count);
};

inline backoff::backoff()
	: spin_count(1)
{}

#pragma optimize("", off)
inline void backoff::spin(size_t count)
{
	for (size_t i = 0; i < count; i++) {}
}
#pragma optimize("", on)

inline void backoff::pause()
{
	if (spin_count <= threshold)
	{
		spin(spin_count);
		spin_count *= 2;
	}

	else
		std::this_thread::yield();
}

inline bool backoff::try_pause()
{
	if (spin_count <= threshold)
	{
		spin(spin_count);
		spin_count *= 2;
		return true;
	}

	return false;
}

inline void backoff::reset()
{
	spin_count = 1;
}

}
