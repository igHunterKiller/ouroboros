// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A small class that spawns a thread to monitor CPU usage by the calling 
// process.

#pragma once
#include <oSystem/debugger.h>
#include <oConcurrency/mutex.h>
#include <oSystem/process.h>
#include <oConcurrency/backoff.h>
#include <oMath/moving_average.h>
#include <thread>

namespace ouro {

class process_stats_monitor
{
public:
	struct info
	{
		info()
			: average_usage(0.0f)
			, low_usage(0.0f)
			, high_usage(0.0f)
		{}

		float average_usage;
		float low_usage;
		float high_usage;
	};

	inline process_stats_monitor(process::id _ProcessID = process::id()
		, std::chrono::milliseconds _PollRate = std::chrono::milliseconds(1000))
			: PID(_ProcessID == process::id() ? this_process::get_id() : _ProcessID)
			, Done(true)
			, PollRate(_PollRate)
	{
		reset();
		Thread = std::thread(&process_stats_monitor::thread_proc, this);
		backoff bo;
		while (Done)
			bo.pause();
	}

	inline process_stats_monitor(process_stats_monitor&& _That)
	{
		operator=(std::move(_That));
	}

	inline ~process_stats_monitor()
	{
		join();
	}

	inline process_stats_monitor& operator=(process_stats_monitor&& _That)
	{
		if (this != &_That)
		{
			Stats = std::move(_That.Stats);
			PreviousSystemTime = std::move(_That.PreviousSystemTime);
			PreviousProcessTime = std::move(_That.PreviousProcessTime);
			MA = std::move(_That.MA);
			PollRate = std::move(_That.PollRate);
			PID = std::move(_That.PID);
			Done = std::move(_That.Done);
			Thread = std::move(_That.Thread);
		}
		return *this;
	}

	inline info get_info() const
	{
		shared_lock<shared_mutex> lock(const_cast<shared_mutex&>(StatsMutex));
		return Stats;
	}

	inline void reset()
	{
		Stats.average_usage = 0.0f;
		Stats.low_usage = 100.0f;
		Stats.high_usage = 0.0f;
		MA = moving_average<double>();
		PreviousSystemTime = 0;
		PreviousProcessTime = 0;
	}

private:
	inline void thread_proc()
	{
		debugger::thread_name("process_stats_monitor");
		Done = false;

		process::cpu_usage(PID, &PreviousSystemTime, &PreviousProcessTime);
		do
		{
			std::this_thread::sleep_for(PollRate);
			double usage = process::cpu_usage(PID, &PreviousSystemTime, &PreviousProcessTime);
			float usagef = static_cast<float>(usage);
			float avg = static_cast<float>(MA.calculate(usage));
			ouro::lock_guard<shared_mutex> lock(StatsMutex);
			Stats.average_usage = avg;
			Stats.low_usage = __min(Stats.low_usage, usagef);
			Stats.high_usage = __max(Stats.high_usage, usagef);

		} while (!Done);
	}

	inline bool joinable() const
	{
		return Thread.joinable();
	}

	inline void join()
	{
		if (joinable())
		{
			Done = true;
			Thread.join();
		}
	}

	shared_mutex StatsMutex;
	info Stats;
	unsigned long long PreviousSystemTime;
	unsigned long long PreviousProcessTime;
	moving_average<double> MA;
	std::chrono::milliseconds PollRate;
	process::id PID;
	bool Done;
	std::thread Thread;
};

}
