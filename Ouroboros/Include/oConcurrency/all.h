// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Convenience "all headers" header for precompiled header files. Do NOT use 
// this to be lazy when including headers in .cpp files. Be explicit.
#pragma once
#include <oConcurrency/backoff.h>
#include <oConcurrency/concurrency.h>
#include <oConcurrency/concurrent_hash_map.h>
#include <oConcurrency/concurrent_queue.h>
#include <oConcurrency/concurrent_queue_opt.h>
#include <oConcurrency/concurrent_stack.h>
#include <oConcurrency/coroutine.h>
#include <oConcurrency/countdown_latch.h>
#include <oConcurrency/event.h>
#include <oConcurrency/future.h>
#include <oConcurrency/lock_free_queue.h>
#include <oConcurrency/mutex.h>
#include <oConcurrency/plan_thread.h>
#include <oConcurrency/tagged_pointer.h>
#include <oConcurrency/threadpool.h>
