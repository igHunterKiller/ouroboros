// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oConcurrency/mutex.h>
#include <oConcurrency/backoff.h>
#include <stdexcept>
#include <Windows.h>

#if NTDDI_VERSION >= NTDDI_WIN7
	#define oHAS_SLIM_TRY_LOCK
#endif

static_assert(sizeof(ouro::recursive_mutex) == sizeof(CRITICAL_SECTION), "size mismatch");
#ifdef _DEBUG
	static_assert(sizeof(ouro::mutex) == sizeof(SRWLOCK) + sizeof(std::thread::id), "size mismatch");
#else
	static_assert(sizeof(ouro::mutex) == sizeof(SRWLOCK), "size mismatch");
#endif

#ifdef _DEBUG
	#define ASSIGN_TID() tid = std::this_thread::get_id()
	#define ASSIGN_TID_CHECKED() do { if (footprint && tid == std::this_thread::get_id()) { throw std::logic_error("non-recursive already locked on this thread"); } tid = std::this_thread::get_id(); } while(false)
	#define CLEAR_TID()	tid = std::thread::id()
	#ifdef oHAS_SLIM_TRY_LOCK
		#define CHECK_UNLOCKED() if (!try_lock()) throw std::logic_error("mutex locked on destruction")
	#else
		#define CHECK_UNLOCKED()
	#endif
#else
	#define ASSIGN_TID()
	#define ASSIGN_TID_CHECKED()
	#define CLEAR_TID()
	#define CHECK_UNLOCKED()
#endif

namespace ouro {

	const adopt_lock_t adopt_lock;
	const defer_lock_t defer_lock;
	const try_to_lock_t try_to_lock;

mutex::mutex()
{
	InitializeSRWLock((PSRWLOCK)&footprint);
}

mutex::~mutex()
{
	CHECK_UNLOCKED();
}

mutex::native_handle_type mutex::native_handle()
{
	return (PSRWLOCK)&footprint;
}

void mutex::lock()
{
	ASSIGN_TID_CHECKED();
	AcquireSRWLockExclusive((PSRWLOCK)&footprint);
}

bool mutex::try_lock()
{
	#ifdef oHAS_SLIM_TRY_LOCK
		return !!TryAcquireSRWLockExclusive((PSRWLOCK)&footprint);
	#else
		return false;
	#endif
}

void mutex::unlock()
{
	CLEAR_TID();
	ReleaseSRWLockExclusive((PSRWLOCK)&footprint);
}

void shared_mutex::lock_shared()
{
	ASSIGN_TID_CHECKED();
	AcquireSRWLockShared((PSRWLOCK)&footprint);
}

bool shared_mutex::try_lock_shared()
{
	#ifdef oHAS_SLIM_TRY_LOCK
		return !!TryAcquireSRWLockShared((PSRWLOCK)&footprint);
	#else
		return false;
	#endif
}

void shared_mutex::unlock_shared()
{
	CLEAR_TID();
	ReleaseSRWLockShared((PSRWLOCK)&footprint);
}

recursive_mutex::recursive_mutex()
{
	InitializeCriticalSection((LPCRITICAL_SECTION)footprint);
}

recursive_mutex::~recursive_mutex()
{
	CHECK_UNLOCKED();
	DeleteCriticalSection((LPCRITICAL_SECTION)footprint);
}

recursive_mutex::native_handle_type recursive_mutex::native_handle()
{
	return (LPCRITICAL_SECTION)footprint;
}

void recursive_mutex::lock()
{
	EnterCriticalSection((LPCRITICAL_SECTION)footprint);
}

bool recursive_mutex::try_lock()
{
	return !!TryEnterCriticalSection((LPCRITICAL_SECTION)footprint);
}

void recursive_mutex::unlock()
{
	LeaveCriticalSection((LPCRITICAL_SECTION)footprint);
}

bool timed_mutex::try_lock_for(unsigned int _TimeoutMS)
{
	// Based on:
	// http://software.intel.com/en-us/blogs/2008/09/17/pondering-timed-mutex/

	ouro::backoff bo;

	do 
	{
		if (try_lock())
			return true;

		if (!bo.try_pause())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			_TimeoutMS -= 10;
			bo.reset();
		}

	} while ((int)_TimeoutMS > 0);

	return false;
}

once_flag::once_flag()
	: footprint(0)
{
	InitOnceInitialize((PINIT_ONCE)footprint);
}

BOOL CALLBACK InitOnceCallback(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
	std::function<void()>* pFN = (std::function<void()>*)Parameter;
	(*pFN)();
	return TRUE;
}

void call_once(once_flag& flag, const std::function<void()>& fn)
{
	InitOnceExecuteOnce(*(PINIT_ONCE*)&flag, InitOnceCallback, (PVOID)&fn, nullptr);
}

}
