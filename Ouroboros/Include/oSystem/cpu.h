// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// inspect the features of the current cpu

#pragma once
#include <functional>
#include <cstdint>

namespace ouro { namespace cpu { 

enum class type
{
	unknown,
	x86,
	x64,
	ia64,
	arm,
};

enum class support
{
	none,          // the feature does not exist
	not_found,     // the feature is not exposed
	hardware_only, // no platform support/API exposure
	full,          // both the current platform and HW support the feature
};

struct cache_info_t
{
	uint32_t size;
	uint32_t line_size;
	uint32_t associativity;
};

struct info_t
{
	type type;
	uint32_t processor_count;
	uint32_t processor_package_count;
	uint32_t hardware_thread_count;
	cache_info_t data_cache[3];
	cache_info_t instruction_cache[3];
	char string[16];
	char brand_string[64];
};

info_t get_info();

// enumerates each feature. Handler should return true to continue, false to exit early
typedef bool (*enumerator_fn)(const char* feature, const support& support, void* user);
void enumerate_features(enumerator_fn enumerator, void* user);

}}
