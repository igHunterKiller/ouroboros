// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMesh/obj.h>
#include <oCore/finally.h>
#include <oCore/timer.h>
#include <oString/fixed_string.h>

#include "obj_test.h"

using namespace ouro;

static void test_correctness(tests::obj_test& expected, const mesh::obj& obj)
{
	auto expected_info = expected.info();
	auto expected_data = expected.source();

	oCHECK(!strcmp(expected.mtl_path(), obj.mtl_path()), "MaterialLibraryPath \"%s\" (should be %s) does not match in obj file \"%s\"", obj.mtl_path(), expected.mtl_path(), obj.mtl_path());
	
	oCHECK(expected_info.num_vertices == obj.num_vertices(), "Position counts do not match in obj file \"%s\"", obj.obj_path());
	for (uint32_t i = 0; i < obj.num_vertices(); i++)
	{
		oCHECK(equal(expected_data.positions [i], obj.positions()[i]), "Position %u does not match in obj file \"%s\"", i, obj.obj_path());
		oCHECK(equal(expected_data.texcoords3[i], obj.texcoords()[i]), "Texcoord %u does not match in obj file \"%s\"", i, obj.obj_path());
		oCHECK(equal(expected_data.normals   [i], obj.normals()  [i]), "Normal %u does not match in obj file \"%s\"",   i, obj.obj_path());
	}
	
	oCHECK(expected_info.num_indices == obj.num_indices(), "Index counts do not match in obj file \"%s\"", obj.obj_path());
	for (uint i = 0; i < obj.num_indices(); i++)
		oCHECK(equal(expected_data.indices32[i], obj.indices()[i]), "Index %u does not match in obj file \"%s\"", i, obj.obj_path());

	oCHECK(expected_info.num_subsets == obj.num_groups(), "Group counts do not match in obj file \"%s\"", obj.obj_path());
	for (uint i = 0; i < obj.num_groups(); i++)
	{
		oCHECK(!strcmp(expected.groups()[i].name,     obj.groups()[i].name    ), "Group %u does not match in obj file \"%s\"", i, obj.obj_path());
		oCHECK(!strcmp(expected.groups()[i].material, obj.groups()[i].material), "Group %u does not match in obj file \"%s\"", i, obj.obj_path());
		oCHECK(expected_data.subsets[i].start_index == obj.groups()[i].start_index, "start_index %u does not match in obj file \"%s\"", i, obj.obj_path());
		oCHECK(expected_data.subsets[i].num_indices == obj.groups()[i].num_indices, "num_indices %u does not match in obj file \"%s\"", i, obj.obj_path());
	}
}

static void obj_load(unit_test::services& services, const char* path, double* out_load_time = nullptr)
{
	double start = timer::now();
	auto b = services.load_buffer(path);

	mesh::obj::init_t init;
	mesh::obj obj(init, path, b, b.size());

	if (out_load_time)
		*out_load_time = timer::now() - start;
}

oTEST(oMesh_obj)
{
	// Correctness
	{
		std::shared_ptr<tests::obj_test> test = tests::obj_test::make(tests::obj_test::cube);
		mesh::obj obj(mesh::obj::init_t(), "Correctness (cube) obj", test->file_contents(), 1 + strlen(test->file_contents()));
		test_correctness(*test.get(), obj);
	}

	// Support for negative indices
	{
		obj_load(srv, "Test/Geometry/hunter.obj");
	}

	// Performance
	{
		static const char* kBenchmarkPath = "Test/Geometry/buddha.obj";
		double load_time = 0.0;
		obj_load(srv, kBenchmarkPath, &load_time);

		sstring time;
		format_duration(time, load_time, true);
		srv.status("%s to load benchmark file %s", time.c_str(), kBenchmarkPath);
	}
}
