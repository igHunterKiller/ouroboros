// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "obj_test.h"
#include <oCore/assert.h>
#include <oCore/countof.h>
#include <oString/string.h>

namespace ouro { namespace tests {

class obj_test_cube : public obj_test
{
public:
	obj_test_cube()
	{
		init_groups();
	}

	mesh::info_t info() const override
	{
		mesh::info_t i;

		auto extents        = float3(0.5f);
		i.num_indices       = countof(sIndices);
		i.num_vertices      = countof(sPositions);
		i.num_subsets       = countof(sSubsets);
		i.num_slots         = (uint8_t)mesh::layout_slots(mesh::basic::wavefront_obj);
		i.log2scale         = mesh::calc_log2scale(extents);
		i.primitive_type    = mesh::primitive_type::triangles;
		i.face_type         = mesh::face_type::front_ccw;
		i.flags             = 0;
		i.bounding_sphere   = float4(0.0f, 0.0f, 0.0f, length(extents));
		i.extents           = extents;
		i.avg_edge_length   = 1.0f;
		i.lod_distances     = float4(0.0f);
		i.avg_texel_density = float2(1.0f);
		i.layout            = mesh::layout(mesh::basic::wavefront_obj);
		i.lods              = mesh::default_lods(i.num_subsets);

		return i;
	}

	mesh::const_source_t source() const override
	{
		mesh::const_source_t s;
		s.indices32  = sIndices;
		s.positions  = sPositions;
		s.texcoords3 = sTexcoords;
		s.normals    = sNormals;
		s.subsets    = sSubsets;
		return s;		
	}

	const char* obj_path() const override { return "cube.obj"; }
	const char* mtl_path() const override { return "cube.mtl"; }

	const mesh::obj::group_t* groups() const override
	{
		return sGroups;
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
			"	f 5/1/1 7/3/1 3/4/1\n" \
			"	f 1/2/1 5/1/1 3/4/1\n" \
			"\n" \
			"	g Right\n" \
			"	f 6/2/2 4/3/2 8/4/2\n" \
			"	f 2/1/2 4/3/2 6/2/2\n" \
			"\n" \
			"	g Top\n" \
			"	f 3/1/3 7/3/3 8/4/3\n" \
			"	f 3/1/3 8/4/3 4/2/3\n" \
			"\n" \
			"	g Bottom\n" \
			"	f 1/3/4 6/2/4 5/1/4\n" \
			"	f 2/4/4 6/2/4 1/3/4\n" \
			"\n" \
			"	g Near\n" \
			"	f 1/1/5 3/3/5 2/2/5\n" \
			"	f 2/2/5 3/3/5 4/4/5\n" \
			"\n" \
			"	g Far\n" \
			"	f 5/2/6 6/1/6 7/4/6\n" \
			"	f 6/1/6 8/3/6 7/4/6\n";
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
				strlcpy(g.name, sGroupNames[i]);
				strlcpy(g.material, "Body");

				sSubsets[i].start_index  = i * 2 * 3;
				sSubsets[i].num_indices  = 2 * 3;
				sSubsets[i].start_vertex = 0;
				sSubsets[i].subset_flags = 0;
				sSubsets[i].unused       = 0;
				sSubsets[i].material_id  = i;
				i++;
			}

			InitGroupsDone = true;
		}
	}
};

const float3 obj_test_cube::sPositions[24] = 
{
	float3(-0.500000000f, -0.500000000f, 0.500000000f),
	float3(-0.500000000f, 0.500000000f, 0.500000000f),
	float3(-0.500000000f, 0.500000000f, -0.500000000f),
	float3(-0.500000000f, -0.500000000f, -0.500000000f),
	float3(0.500000000f, -0.500000000f, 0.500000000f),
	float3(0.500000000f, 0.500000000f, -0.500000000f),
	float3(0.500000000f, 0.500000000f, 0.500000000f),
	float3(0.500000000f, -0.500000000f, -0.500000000f),
	float3(-0.500000000f, 0.500000000f, -0.500000000f),
	float3(-0.500000000f, 0.500000000f, 0.500000000f),
	float3(0.500000000f, 0.500000000f, 0.500000000f),
	float3(0.500000000f, 0.500000000f, -0.500000000f),
	float3(-0.500000000f, -0.500000000f, -0.500000000f),
	float3(0.500000000f, -0.500000000f, 0.500000000f),
	float3(-0.500000000f, -0.500000000f, 0.500000000f),
	float3(0.500000000f, -0.500000000f, -0.500000000f),
	float3(-0.500000000f, -0.500000000f, -0.500000000f),
	float3(-0.500000000f, 0.500000000f, -0.500000000f),
	float3(0.500000000f, -0.500000000f, -0.500000000f),
	float3(0.500000000f, 0.500000000f, -0.500000000f),
	float3(-0.500000000f, -0.500000000f, 0.500000000f),
	float3(0.500000000f, -0.500000000f, 0.500000000f),
	float3(-0.500000000f, 0.500000000f, 0.500000000f),
	float3(0.500000000f, 0.500000000f, 0.500000000f),
};

const float3 obj_test_cube::sTexcoords[24] = 
{
	float3(0.000000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 1.00000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
};

const float3 obj_test_cube::sNormals[24] = 
{
	float3(-1.00000000f, 0.000000000f, 0.000000000f),
	float3(-1.00000000f, 0.000000000f, 0.000000000f),
	float3(-1.00000000f, 0.000000000f, 0.000000000f),
	float3(-1.00000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(1.00000000f, 0.000000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, 1.00000000f, 0.000000000f),
	float3(0.000000000f, -1.00000000f, 0.000000000f),
	float3(0.000000000f, -1.00000000f, 0.000000000f),
	float3(0.000000000f, -1.00000000f, 0.000000000f),
	float3(0.000000000f, -1.00000000f, 0.000000000f),
	float3(0.000000000f, 0.000000000f, -1.00000000f),
	float3(0.000000000f, 0.000000000f, -1.00000000f),
	float3(0.000000000f, 0.000000000f, -1.00000000f),
	float3(0.000000000f, 0.000000000f, -1.00000000f),
	float3(0.000000000f, 0.000000000f, 1.00000000f),
	float3(0.000000000f, 0.000000000f, 1.00000000f),
	float3(0.000000000f, 0.000000000f, 1.00000000f),
	float3(0.000000000f, 0.000000000f, 1.00000000f),
};

const uint obj_test_cube::sIndices[36] =
{
	0,1,2,3,0,2,
	4,5,6,7,5,4,
	8,9,10,8,10,11,
	12,13,14,15,13,12,
	16,17,18,18,17,19,
	20,21,22,21,23,22,
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

	oThrow(std::errc::invalid_argument, "invalid obj_test");
}

}}

