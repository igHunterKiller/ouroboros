// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMesh/obj.h>
#include <oCore/finally.h>
#include <oCore/timer.h>

#include "obj_test.h"

using namespace ouro;

static void test_correctness(const std::shared_ptr<tests::obj_test>& _Expected, const std::shared_ptr<mesh::obj::mesh>& _OBJ)
{
	mesh::obj::info_t expectedInfo = _Expected->get_info();
	mesh::obj::info_t objInfo = _OBJ->info();

	oCHECK(!strcmp(expectedInfo.mtl_path, objInfo.mtl_path), "MaterialLibraryPath \"%s\" (should be %s) does not match in obj file \"%s\"", objInfo.mtl_path.c_str(), expectedInfo.mtl_path.c_str(), objInfo.obj_path.c_str());
	
	oCHECK(expectedInfo.mesh_info.num_vertices == objInfo.mesh_info.num_vertices, "Position counts do not match in obj file \"%s\"", objInfo.obj_path.c_str());
	for (uint i = 0; i < objInfo.mesh_info.num_vertices; i++)
	{
		oCHECK(equal(expectedInfo.positions[i], objInfo.positions[i]), "Position %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
		oCHECK(equal(expectedInfo.normals[i], objInfo.normals[i]), "Normal %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
		oCHECK(equal(expectedInfo.texcoords[i], objInfo.texcoords[i]), "Texcoord %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
	}
	
	oCHECK(expectedInfo.mesh_info.num_indices == objInfo.mesh_info.num_indices, "Index counts do not match in obj file \"%s\"", objInfo.obj_path.c_str());
	for (uint i = 0; i < objInfo.mesh_info.num_indices; i++)
		oCHECK(equal(expectedInfo.indices[i], objInfo.indices[i]), "Index %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());

	oCHECK(expectedInfo.mesh_info.num_subsets == objInfo.mesh_info.num_subsets, "Group counts do not match in obj file \"%s\"", objInfo.obj_path.c_str());
	for (uint i = 0; i < objInfo.mesh_info.num_subsets; i++)
	{
		oCHECK(!strcmp(expectedInfo.groups[i].group_name.c_str(), objInfo.groups[i].group_name.c_str()), "Group %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
		oCHECK(!strcmp(expectedInfo.groups[i].material_name.c_str(), objInfo.groups[i].material_name.c_str()), "Group %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
		oCHECK(expectedInfo.subsets[i].start_index == objInfo.subsets[i].start_index, "start_index %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
		oCHECK(expectedInfo.subsets[i].num_indices == objInfo.subsets[i].num_indices, "num_indices %u does not match in obj file \"%s\"", i, objInfo.obj_path.c_str());
	}
}

static void obj_load(unit_test::services& services, const char* _Path, double* _pLoadTime = nullptr)
{
	double start = timer::now();
	auto b = services.load_buffer(_Path);

	mesh::obj::init_t init;
	init.calc_normals_on_error = false; // buddha doesn't have normals and is 300k faces... let's not sit in the test suite calculating such a large test case
	std::shared_ptr<mesh::obj::mesh> obj = mesh::obj::mesh::make(init, _Path, b);

	if (_pLoadTime)
		*_pLoadTime = timer::now() - start;
}

oTEST(oMesh_obj)
{
	// Correctness
	{
		std::shared_ptr<tests::obj_test> test = tests::obj_test::make(tests::obj_test::cube);
		std::shared_ptr<mesh::obj::mesh> obj = mesh::obj::mesh::make(mesh::obj::init_t(), "Correctness (cube) obj", test->file_contents());
		test_correctness(test, obj);
	}

	// Support for negative indices
	{
		obj_load(srv, "Test/Geometry/hunter.obj");
	}

	// Performance
	{
		static const char* BenchmarkFilename = "Test/Geometry/buddha.obj";
		double LoadTime = 0.0;
		obj_load(srv, BenchmarkFilename, &LoadTime);

		sstring time;
		format_duration(time, LoadTime, true);
		srv.status("%s to load benchmark file %s", time.c_str(), BenchmarkFilename);
	}
}
