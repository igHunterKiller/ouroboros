// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/timer.h>
#include <oString/atof.h>
#include <oString/string.h>
#include <oMath/equal.h>
#include <array>
#include <chrono>
#include <vector>

using namespace ouro;

oTEST(oString_atof)
{
	static const std::array<const char*, 4> sFloatStrings = 
	{
		"3.1415926535897932384",
		"+0.341251",
		"-0.0959615",
		"3.15819e-06",
	};

	static const std::array<float, 4> sFloats = 
	{
		3.1415926535897932384f,
		0.341251f,
		-0.0959615f,
		3.15819e-06f,
	};

	std::vector<char> buf(3*1024*1024);

	float f;
	for (size_t i = 0; i < sFloats.size(); i++)
	{
		if (!atof(sFloatStrings[i], &f))
		{
			throw std::invalid_argument(std::string("ouro::atof failed on ") + sFloatStrings[i]);
		}

		if (!equal(f, sFloats[i]))
		{
			throw std::logic_error(std::string("ouro::atof failed on ") + sFloatStrings[i]);
		}
	}

	#ifdef _DEBUG // takes too long in debug
		static const size_t kNumFloats = 20000;
	#else
		static const size_t kNumFloats = 200000;
	#endif

	srv.trace("Preparing test data...");
	char* fstr = buf.data();
	char* end = fstr + buf.size();
	for (size_t i = 0; i < kNumFloats; i++)
	{
		float rand01 = (srv.rand() % RAND_MAX) / static_cast<float>(RAND_MAX - 1);
		float f = -1000.0f + (2000.0f * rand01);
		size_t len = snprintf(fstr, std::distance(fstr, end), "%f\n", f);
		fstr += len + 1;
	}

	srv.trace("Benchmarking stdc atof()...");

	std::vector<float> flist;
	flist.reserve(kNumFloats);

	fstr = buf.data();
		
	timer tm;
	while (fstr < end)
	{
		float f = static_cast<float>(::atof(fstr));
		flist.push_back(f);
		fstr += strcspn(fstr, "\n") + 1;
	}
		
	auto atofDuration = tm.millis();
	srv.trace("atof() %.02f ms", atofDuration);

	srv.trace("Benchmarking ouro::atof()...");
	flist.clear();

	fstr = buf.data();
	tm.reset();
	while (fstr < end)
	{
		float f;
		atof(fstr, &f);
		flist.push_back(f);
		fstr += strcspn(fstr, "\n") + 1;
	}
		
	auto ouroAtofDuration = tm.millis();
	srv.status("%.02f v. %.02f ms for %u floats (%.02fx improvement)", atofDuration, ouroAtofDuration, kNumFloats, atofDuration / ouroAtofDuration);
}
