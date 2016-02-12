// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Wrapper around the atrocious std::chrono

#pragma once
#include <chrono>

namespace ouro {

class timer
{
public:
	template<typename T, typename ratioT> static T nowT() { return std::chrono::duration_cast<std::chrono::duration<T, ratioT>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); }
	template<typename T> static T nowT()                  { return std::chrono::duration_cast<std::chrono::duration<T, std::ratio<1>>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); }

	static long long now_us()                             { return nowT<long long, std::micro>(); }
	static double now()                                   { return nowT<double>(); }
	static float now_f()                                  { return nowT<float>(); }
	static double now_ms()                                { return nowT<double, std::milli>(); }
	static float now_msf()                                { return nowT<float, std::milli>(); }
	static unsigned int now_msi()                         { return nowT<unsigned int, std::milli>(); }

	timer()                                               { reset(); }
	void reset()                                          { start_ = now(); }
	double seconds() const                                { return now() - start_; }
	double millis() const                                 { return seconds() * 1000.0; }
	double micros() const                                 { return seconds() * 1000000.0; }
	double seconds_and_reset()                            { double s = start_; reset(); return start_ - s; }
	double millis_and_reset()                             { return seconds_and_reset() * 1000.0; }
	double micros_and_reset()                             { return seconds_and_reset() * 1000000.0; }

private:
	double start_;
};

}
