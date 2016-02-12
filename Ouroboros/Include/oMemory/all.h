// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Convenience "all headers" header for precompiled header files. Do NOT use 
// this to be lazy when including headers in .cpp files. Be explicit.

#pragma once
#include <oMemory/concurrent_linear_allocator.h>
#include <oMemory/concurrent_pool.h>
#include <oMemory/concurrent_object_pool.h>
#include <oMemory/djb2.h>
#include <oMemory/fnv1a.h>
#include <oMemory/linear_allocator.h>
#include <oMemory/memory.h>
#include <oMemory/murmur3.h>
#include <oMemory/object_pool.h>
#include <oMemory/pool.h>
#include <oMemory/spatial_hash.h>
#include <oMemory/std_allocator.h>
#include <oMemory/std_linear_allocator.h>
#include <oMemory/wang.h>
#include <oMemory/xxhash.h>
