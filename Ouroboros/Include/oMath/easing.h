// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Math functions for advanced interpolation (easing) values between two points
#pragma once
#include <oMath/hlsl.h>
#include <oMath/floats.h>

namespace ouro {

// Robert Penner
// http://robertpenner.com/easing/  https://github.com/jesusgollonet/ofpennereasing  http://robertpenner.com/easing/penner_chapter7_tweening.pdf

template<typename T> T simple_linear_tween(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { return _Change * _TimeInSeconds/_Duration + _Start; }

template<typename T> T quadratic_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; return _Change * pow(_TimeInSeconds, T(2)) + _Start; }
template<typename T> T quadratic_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; return -_Change * _TimeInSeconds * (_TimeInSeconds - 2) + _Start; }
template<typename T> T quadratic_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration)
{
	_TimeInSeconds /= _Duration / T(2);
	if (_TimeInSeconds < T(1))
		return _Change / T(2) * pow(_TimeInSeconds, T(2)) + _Start;
	_TimeInSeconds--;
	return -_Change / T(2) * (_TimeInSeconds * (_TimeInSeconds - T(2)) - T(1)) + _Start;
}

template<typename T> T cubic_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; return _Change * pow(_TimeInSeconds, T(3)) + _Start; }
template<typename T> T cubic_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; _TimeInSeconds--; return _Change * (pow(_TimeInSeconds, T(3)) + T(1)) + _Start; }
template<typename T> T cubic_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration)
{
	_TimeInSeconds /= _Duration / T(2);
	if (_TimeInSeconds < T(1))
		return _Change / T(2) * pow(_TimeInSeconds, T(3)) + _Start;
	_TimeInSeconds -= T(2);
	return _Change / T(2) * (pow(_TimeInSeconds, T(3)) + T(2)) + _Start;
}

template<typename T> T quartic_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; return _Change * pow(_TimeInSeconds, T(4)) + _Start; }
template<typename T> T quartic_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; _TimeInSeconds--; return -_Change * (pow(_TimeInSeconds, T(4)) - T(1)) + _Start; }
template<typename T> T quartic_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) 
{ 
	_TimeInSeconds /= _Duration / T(2);
	if (_TimeInSeconds < T(1))
		return _Change / T(2) * pow(_TimeInSeconds, T(4)) + _Start;
	_TimeInSeconds -= T(2);
	return -_Change / T(2) * (pow(_TimeInSeconds, T(4)) - T(2)) + _Start;
}

template<typename T> T quintic_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; return _Change * pow(_TimeInSeconds, T(5)) + _Start; }
template<typename T> T quintic_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; _TimeInSeconds--; return _Change * (pow(_TimeInSeconds, T(5)) + T(1)) + _Start; }
template<typename T> T quintic_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration)
{
	_TimeInSeconds /= _Duration / T(2);
	if (_TimeInSeconds < T(1))
		return _Change / T(2) * pow(_TimeInSeconds, T(5)) + _Start;
	_TimeInSeconds -= T(2);
	return _Change / T(2) * (pow(_TimeInSeconds, T(5)) + T(2)) + _Start;
}

template<typename T> T sin_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { return -_Change * cos(_TimeInSeconds/_Duration * T(oPI) / T(2)) + _Change + _Start; }
template<typename T> T sin_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { return _Change * sin(_TimeInSeconds/_Duration * T(oPI) / T(2)) + _Start; }
template<typename T> T sin_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { return-_Change/T(2) * (cos(T(oPI)*_TimeInSeconds/_Duration) - T(1)) + _Start; }

template<typename T> T exponential_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { return _Change * pow(T(2), T(10) * (_TimeInSeconds/_Duration - T(1))) + _Start; }
template<typename T> T exponential_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { return _Change * (-pow(T(2), T(-10) * _TimeInSeconds/_Duration) + T(1)) + _Start; }
template<typename T> T exponential_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration)
{
	_TimeInSeconds /= _Duration / T(2);
	if (_TimeInSeconds < 1)
		return _Change/T(2) * pow(T(2), T(10) * (_TimeInSeconds - T(1))) + _Start;
	_TimeInSeconds--;
	return _Change/T(2) * (-pow(T(2), T(-10) * _TimeInSeconds) + T(2)) + _Start;
}

template<typename T> T circular_ease_in(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; return (T)(-_Change * (sqrt(1 - pow(_TimeInSeconds, 2)) - 1) + _Start); }
template<typename T> T circular_ease_out(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration) { _TimeInSeconds /= _Duration; _TimeInSeconds--; return (T)(_Change * sqrt(1 - pow(_TimeInSeconds, 2)) + _Start); }
template<typename T> T circular_ease_inout(float _TimeInSeconds, const T& _Start, const T& _Change, float _Duration)
{
	_TimeInSeconds /= _Duration / T(2);
	if (_TimeInSeconds < T(1))
		return -_Change/T(2) * (sqrt(T(1) - pow(_TimeInSeconds, T(2))) - T(1)) + _Start;
	_TimeInSeconds -= T(2);
	return _Change/T(2) * (sqrt(T(1) - pow(_TimeInSeconds, T(2))) + T(1)) + _Start;
}

}
