// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oBase/scoped_timer.h>
#include <oSurface/resize.h>
#include <oSurface/codec.h>
#include <vector>

using namespace ouro;

static void TESTsurface_resize_test_size(unit_test::services& services
	, const surface::image& _Buffer
	, surface::filter _Filter
	, const int3& _NewSize
	, int _NthImage)
{
	auto srcInfo = _Buffer.info();
	auto destInfo = srcInfo;
	destInfo.dimensions = _NewSize;
	surface::image dst(destInfo);
	{
		surface::shared_lock lock(_Buffer);
		surface::lock_guard lock2(dst);

		scoped_timer timer("resize time");
		surface::resize(srcInfo, lock.mapped, destInfo, lock2.mapped, _Filter);
	}

	services.check(dst, _NthImage);
}

static void TESTsurface_resize_test_filter(unit_test::services& srv
	, const surface::image& _Buffer
	, surface::filter _Filter
	, int _NthImage)
{
	TESTsurface_resize_test_size(srv, _Buffer, _Filter, _Buffer.info().dimensions * int3(2,2,1), _NthImage);
	TESTsurface_resize_test_size(srv, _Buffer, _Filter, _Buffer.info().dimensions / int3(2,2,1), _NthImage+1);
}

oTEST(oSurface_surface_resize)
{
	auto b = srv.load_buffer("Test/Textures/lena_1.png");
	surface::image s = surface::decode(b, b.size());

	int NthImage = 0;
	for (int i = 0; i < (int)surface::filter::count; i++)
	{
		TESTsurface_resize_test_filter(srv, s, (surface::filter)i, NthImage);
		NthImage += 2;
	}
}
