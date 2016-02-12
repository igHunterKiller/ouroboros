// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "obj_test.h"
#include <oCore/countof.h>

namespace ouro { namespace tests {

class obj_test_cube : public obj_test
{
public:
	obj_test_cube()
	{
		init_groups();
	}

	mesh::obj::info_t get_info() const override
	{
		mesh::obj::info_t i;

		i.obj_path = "cube.obj";
		i.mtl_path = "cube.mtl";
		i.groups = sGroups;
		i.subsets = sSubsets;
		i.indices = sIndices;
		i.positions = sPositions;
		i.normals = sNormals;
		i.texcoords = sTexcoords;

		i.mesh_info.num_indices = countof(sIndices);
		i.mesh_info.num_vertices = countof(sPositions);
		i.mesh_info.num_subsets = countof(sSubsets);
		i.mesh_info.log2scale = 0;
		i.mesh_info.primitive_type = mesh::primitive_type::triangles;
		i.mesh_info.face_type = mesh::face_type::unknown;
		i.mesh_info.flags = 0;

		i.mesh_info.bounding_sphere = float4(0.0f, 0.0f, 0.0f, length(float3(0.5f)));
		i.mesh_info.extents = float3(0.5f);
		i.mesh_info.avg_edge_length = 1.0f;
		i.mesh_info.avg_texel_density = float2(1.0f, 1.0f);
		i.mesh_info.layout[0] = mesh::celement_t(mesh::element_semantic::position, 0, surface::format::r32g32b32_float,    0);
		i.mesh_info.layout[1] = mesh::celement_t(mesh::element_semantic::normal,   0, surface::format::r32g32b32_float,    1);
		i.mesh_info.layout[2] = mesh::celement_t(mesh::element_semantic::tangent,  0, surface::format::r32g32b32a32_float, 2);
		i.mesh_info.lods[0].opaque_color.start_subset = 0;
		i.mesh_info.lods[0].opaque_color.num_subsets = i.mesh_info.num_subsets;
		i.mesh_info.lods[0].opaque_shadow.start_subset = 0;
		i.mesh_info.lods[0].opaque_shadow.num_subsets = i.mesh_info.num_subsets;
		i.mesh_info.lods[0].collision.start_subset = 0;
		i.mesh_info.lods[0].collision.num_subsets = i.mesh_info.num_subsets;

		return i;
	}

	const char* file_contents() const override
	{
		return
			"# Simple Unit Cube\n" \
			"mtllib cube.mtl\n" \
			"\n" \
			"	v -0.5 -0.5 -0.5\n" \
			"	v 0.5 -0.5 -0.5\n" \
			"	v -0.5 0.5 -0.5\n" \
			"	v 0.5 0.5 -0.5\n" \
			"	v -0.5 -0.5 0.5\n" \
			"	v 0.5 -0.5 0.5\n" \
			"	v -0.5 0.5 0.5\n" \
			"	v 0.5 0.5 0.5\n" \
			"\n" \
			"	vn -1.0 0.0 0.0\n" \
			"	vn 1.0 0.0 0.0\n" \
			"	vn 0.0 1.0 0.0\n" \
			"	vn 0.0 -1.0 0.0\n" \
			"	vn 0.0 0.0 -1.0\n" \
			"	vn 0.0 0.0 1.0\n" \
			"\n" \
			"	vt 0.0 0.0\n" \
			"	vt 1.0 0.0\n" \
			"	vt 0.0 1.0\n" \
			"	vt 1.0 1.0\n" \
			"\n" \
			"	usemtl Body\n" \
			"	g Left\n" \
			"	f 5/3/1 3/4/1 7/1/1\n" \
			"	f 1/2/1 3/4/1 5/3/1\n" \
			"\n" \
			"	g Right\n" \
			"	f 6/2/2 8/4/2 4/3/2\n" \
			"	f 2/1/2 6/2/2 4/3/2\n" \
			"\n" \
			"	g Top\n" \
			"	f 3/1/3 8/4/3 7/3/3\n" \
			"	f 3/1/3 4/2/3 8/4/3\n" \
			"\n" \
			"	g Bottom\n" \
			"	f 1/3/4 5/1/4 6/2/4\n" \
			"	f 2/4/4 1/3/4 6/2/4\n" \
			"\n" \
			"	g Near\n" \
			"	f 1/1/5 2/2/5 3/3/5\n" \
			"	f 2/2/5 4/4/5 3/3/5\n" \
			"\n" \
			"	g Far\n" \
			"	f 5/2/6 7/4/6 6/1/6\n" \
			"	f 6/1/6 7/4/6 8/3/6\n";
	}

private:

	static const float3 sPositions[24];
	static const float3 sTexcoords[24];
	static const float3 sNormals[24];
	static const uint32_t sIndices[36];
	static const char* sGroupNames[6];
	static mesh::obj::group_t sGroups[6];
	static mesh::subset_t sSubsets[6];
	static bool InitGroupsDone;
	static void init_groups()
	{
		if (!InitGroupsDone)
		{
			int i = 0;
			for (auto& g : sGroups)
			{
				g.group_name = sGroupNames[i];
				g.material_name = "Body";

				sSubsets[i].start_index = i * 2 * 3;
				sSubsets[i].num_indices = 2 * 3;
				sSubsets[i].start_vertex = 0;
				sSubsets[i].num_vertices = countof(sPositions);
				sSubsets[i].subset_flags = 0;
				sSubsets[i].material_id = i;
				i++;
			}

			InitGroupsDone = true;
		}
	}
};

const float3 obj_test_cube::sPositions[24] = 
{
	float3(-0.500000f, -0.500000f, 0.500000f),
	float3(-0.500000f, 0.500000f, -0.500000f),
	float3(-0.500000f, 0.500000f, 0.500000f),
	float3(-0.500000f, -0.500000f, -0.500000f),
	float3(0.500000f, -0.500000f, 0.500000f),
	float3(0.500000f, 0.500000f, 0.500000f),
	float3(0.500000f, 0.500000f, -0.500000f),
	float3(0.500000f, -0.500000f, -0.500000f),
	float3(-0.500000f, 0.500000f, -0.500000f),
	float3(0.500000f, 0.500000f, 0.500000f),
	float3(-0.500000f, 0.500000f, 0.500000f),
	float3(0.500000f, 0.500000f, -0.500000f),
	float3(-0.500000f, -0.500000f, -0.500000f),
	float3(-0.500000f, -0.500000f, 0.500000f),
	float3(0.500000f, -0.500000f, 0.500000f),
	float3(0.500000f, -0.500000f, -0.500000f),
	float3(-0.500000f, -0.500000f, -0.500000f),
	float3(0.500000f, -0.500000f, -0.500000f),
	float3(-0.500000f, 0.500000f, -0.500000f),
	float3(0.500000f, 0.500000f, -0.500000f),
	float3(-0.500000f, -0.500000f, 0.500000f),
	float3(-0.500000f, 0.500000f, 0.500000f),
	float3(0.500000f, -0.500000f, 0.500000f),
	float3(0.500000f, 0.500000f, 0.500000f),
};

const float3 obj_test_cube::sTexcoords[24] = 
{
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(1.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(1.000000f, 1.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
};

const float3 obj_test_cube::sNormals[24] = 
{
	float3(-1.000000f, 0.000000f, 0.000000f),
	float3(-1.000000f, 0.000000f, 0.000000f),
	float3(-1.000000f, 0.000000f, 0.000000f),
	float3(-1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(1.000000f, 0.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, 1.000000f, 0.000000f),
	float3(0.000000f, -1.000000f, 0.000000f),
	float3(0.000000f, -1.000000f, 0.000000f),
	float3(0.000000f, -1.000000f, 0.000000f),
	float3(0.000000f, -1.000000f, 0.000000f),
	float3(0.000000f, 0.000000f, -1.000000f),
	float3(0.000000f, 0.000000f, -1.000000f),
	float3(0.000000f, 0.000000f, -1.000000f),
	float3(0.000000f, 0.000000f, -1.000000f),
	float3(0.000000f, 0.000000f, 1.000000f),
	float3(0.000000f, 0.000000f, 1.000000f),
	float3(0.000000f, 0.000000f, 1.000000f),
	float3(0.000000f, 0.000000f, 1.000000f),
};

const uint obj_test_cube::sIndices[36] =
{
	0,2,1,3,0,1,
	4,6,5,7,6,4,
	8,10,9,8,9,11,
	12,14,13,15,14,12,
	16,18,17,17,18,19,
	20,22,21,22,23,21,
};

const char* obj_test_cube::sGroupNames[6] = 
{
	"Left",
	"Right",
	"Top",
	"Bottom",
	"Near",
	"Far"
};

bool obj_test_cube::InitGroupsDone = false;
mesh::obj::group_t obj_test_cube::sGroups[6];
mesh::subset_t obj_test_cube::sSubsets[6];

std::shared_ptr<obj_test> obj_test::make(which _Which)
{
	switch (_Which)
	{
		case obj_test::cube: return std::make_shared<obj_test_cube>();
		default: break;
	}

	throw std::invalid_argument("invalid obj_test");
}

}}

