// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A small class to encapsulate calculating a cumulative moving average

#pragma once

namespace ouro {

template<typename T>
class moving_average
{
public:
	moving_average()
		: ca_(T(0))
		, count_(T(0))
	{}

	// Given the latest sample value, this returns the moving average for all 
	// values up to the latest specified.
	T calculate(const T& value)
	{
		count_ += T(1);
		ca_ = ca_ + ((value - ca_) / count_);
		return ca_;
	}

private:
	T ca_;
	T count_;
};

}
