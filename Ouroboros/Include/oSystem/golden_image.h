// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This encapsulation defines a process for doing fuzzy compares of test images
// against a collection of known-good (golden) images organzied by bins that
// could affect the validity of an image such as driver, HW, etc.

// This is intended to be used in a unit testing system.

// order of precidence (most-specific first):
// {GoldenDir}/OS/Vendor/HW/Driver
// {GoldenDir}/OS/Vendor/HW
// {GoldenDir}/OS/Vendor
// {GoldenDir}/OS
// {GoldenDir}

#pragma once

#include <oString/path.h>
#include <oSurface/image.h>
#include <oSystem/adapter.h>
#include <array>

namespace ouro { 

class golden_image
{
public:
	struct init_t
	{
		adapter::id adapter_id;
		const char* golden_path_root;
		const char* failed_path_root;
	};

	golden_image() {}
	golden_image(const init_t& init) { initialize(init); }
	~golden_image() { deinitialize(); }

	void initialize(const init_t& init);
	void deinitialize();

	// Compares against the matching golden image. If there is a mismatch this 
	// throws a system_error and generates a failure and diff image.
	void test(const char* test_name, const surface::image& img, uint32_t nth_img, float max_rms_error, uint32_t diff_img_multiplier);

private:

	enum path
	{
		root,
		os,
		vendor,
		hw,
		driver,
		count,
	};

	std::array<path_t, count> golden_;
	std::array<path_t, count> failed_;
};

}
