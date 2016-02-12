// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Simple timer for quick-and-dirty code timing.

#pragma once
#include <oCore/timer.h>
#include <oCore/assert.h>
#include <chrono>
#include <cstdint>

namespace ouro {

class scoped_timer : public timer
{
	// A super-simple benchmarking tool to report to stdout the time since this 
	// timer was last reset. This traces in the dtor, so scoping can be used to 
	// group the benchmarking.

public:

	// name must have a scope greater than this timer
	scoped_timer(const char* name) : name_(name) {}
	~scoped_timer() { trace(); }
	
	inline void trace() { oTRACEA("%s took %.03f sec", name_ ? name_ : "(null)", seconds()); }

private:
	const char* name_;
};

}
