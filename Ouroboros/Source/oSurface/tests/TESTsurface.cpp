// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSurface/surface.h>
#include <oCore/countof.h>

using namespace ouro;

static void TESTsurface_row_pitch(int _Depth, int _ArraySize)
{
	// This tests whether the row pitch remains the same per mip level regardless 
	// of depth or number of slices
	surface::info_t inf;
	inf.dimensions = int3(512, 512, _Depth);
	inf.array_size = _ArraySize;
	inf.format = surface::format::r8g8b8a8_unorm;

	uint RowPitches[10] = { 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4 };

	inf.mip_layout = surface::mip_layout::none;
	for (uint mip_level = 0; mip_level < countof(RowPitches); mip_level++)
	{
		oCHECK(surface::row_pitch(inf, 0) == RowPitches[mip_level], "Test_oSurfaceMipCalcRowPitch(%d, %d) surface::mip_layout::none failed", _Depth, _ArraySize);
		inf.dimensions = surface::dimensions_npot(inf.format, inf.dimensions, 1);
	}
	inf.dimensions = int3(512, 512, _Depth);

	inf.mip_layout = surface::mip_layout::tight;
	for (uint mip_level = 0; mip_level < countof(RowPitches); mip_level++)
		oCHECK(surface::row_pitch(inf, (int)mip_level) == RowPitches[mip_level], "Test_oSurfaceMipCalcRowPitch(%d, %d) oSURFACE_LAYOUT_TIGHT failed", _Depth, _ArraySize);

	inf.mip_layout = surface::mip_layout::below;
	for (uint mip_level = 0; mip_level < countof(RowPitches); mip_level++)
		oCHECK(surface::row_pitch(inf, (int)mip_level) == RowPitches[0], "Test_oSurfaceMipCalcRowPitch(%d, %d) oSURFACE_LAYOUT_BELOW failed", _Depth, _ArraySize);

	inf.mip_layout = surface::mip_layout::right;
	for (uint mip_level = 0; mip_level < countof(RowPitches); mip_level++)
		oCHECK(surface::row_pitch(inf, (int)mip_level) == (RowPitches[0]+RowPitches[1]), "Test_oSurfaceMipCalcRowPitch(%d, %d) oSURFACE_LAYOUT_RIGHT failed", _Depth, _ArraySize);
}

static void TESTsurface_depth_pitch(int _Depth, int _ArraySize)
{
	// This tests whether the depth pitch remains the same per mip level regardless of depth or number of slices
	surface::info_t inf;
	inf.dimensions = uint3(512,512,_Depth);
	inf.array_size = _ArraySize;
	inf.format = surface::format::r8g8b8a8_unorm;

	uint DepthPitches[10] = { 0x100000, 0x40000, 0x10000, 0x4000, 0x1000, 0x400, 0x100, 0x40, 0x10, 0x4 };

	inf.mip_layout = surface::mip_layout::none;
	for (uint mip_level = 0; mip_level < countof(DepthPitches); mip_level++)
	{
		oCHECK(surface::depth_pitch(inf, 0) == DepthPitches[mip_level], "Test_oSurfaceMipCalcDepthPitch() surface::mip_layout::none failed");
		inf.dimensions = surface::dimensions_npot(inf.format, inf.dimensions, 1);
	}
	inf.dimensions = int3(512,512,_Depth);

	inf.mip_layout = surface::mip_layout::tight;
	for (uint mip_level = 0; mip_level < countof(DepthPitches); mip_level++)
		oCHECK(surface::depth_pitch(inf, (int)mip_level) == DepthPitches[mip_level], "Test_oSurfaceMipCalcDepthPitch() oSURFACE_LAYOUT_TIGHT failed");

	inf.mip_layout = surface::mip_layout::below;
	for (uint mip_level = 0; mip_level < countof(DepthPitches); mip_level++)
		oCHECK(surface::depth_pitch(inf, (int)mip_level) == DepthPitches[mip_level] << mip_level, "surface::depth_pitch() oSURFACE_LAYOUT_BELOW failed");

	inf.mip_layout = surface::mip_layout::right;
	for (uint mip_level = 0; mip_level < countof(DepthPitches); mip_level++)
		oCHECK(surface::depth_pitch(inf, (int)mip_level) == (DepthPitches[mip_level] << mip_level) + ((DepthPitches[mip_level] >> 1) << mip_level), "surface::depth_pitch() oSURFACE_LAYOUT_RIGHT failed");
}

static void TESTsurface_slice_pitch(int _ArraySize)
{
	surface::info_t inf;
	inf.dimensions = int3(512,512,1);
	inf.array_size = _ArraySize;
	inf.format = surface::format::r8g8b8a8_unorm;

	if (_ArraySize > 1)
	{
		// This tests whether the slice pitch remains the same regardless of number of slices
		auto inf1 = inf;
		inf1.array_size = 1;

		inf.mip_layout = surface::mip_layout::none;
		oCHECK(surface::slice_pitch(inf) == 0x00100000, "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == surface::depth_pitch(inf), "surface::slice_pitch() failed");
		
		inf.mip_layout = surface::mip_layout::tight;
		oCHECK(surface::slice_pitch(inf) == 0x00155800, "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == (surface::total_size(inf) / _ArraySize), "surface::slice_pitch()==(surface::total_size()/slices) failed");

		inf.mip_layout = surface::mip_layout::below;
		oCHECK(surface::slice_pitch(inf) == 0x00180000, "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == (surface::total_size(inf) / _ArraySize), "surface::slice_pitch()==(surface::total_size()/slices) failed");

		inf.mip_layout = surface::mip_layout::right;
		oCHECK(surface::slice_pitch(inf) == 0x00180000, "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == (surface::total_size(inf) / _ArraySize), "surface::slice_pitch()==(surface::total_size()/slices) failed");
		return;
	}

	// This tests whether the slice pitch is the same as the depth pitch multiplied by the depth
	inf.mip_layout = surface::mip_layout::none;
	for(int depthLevel=0; depthLevel<10; depthLevel++)
	{
		inf.dimensions.z = (int)depthLevel + 1;
		oCHECK(surface::slice_pitch(inf) == (surface::depth_pitch(inf, 0) * inf.dimensions.z), "surface::slice_pitch()==(surface::depth_pitch()*depth) failed");
		oCHECK(surface::slice_pitch(inf) == surface::total_size(inf), "surface::slice_pitch()==surface::total_size() failed");
	}

	// The rest below tests the slice pitches for a number of depth levels, and 3d textures don't support more than one slice
	inf.mip_layout = surface::mip_layout::tight;
	uint SlicePitches[15] = { 0x00155800, 0x00255800, 0x00355800, 0x00495800, 0x00595800, 0x006d5800, 0x007d5800, 0x00925800, 0x00a25800, 0x00b65800, 0x00c65800, 0x00db5800, 0x00eb5800, 0x00ff5800, 0x010f5800 };
	for (uint depthLevel = 0; depthLevel < countof(SlicePitches); depthLevel++)
	{
		inf.dimensions.z = depthLevel + 1;
		oCHECK(surface::slice_pitch(inf) == SlicePitches[depthLevel], "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == surface::total_size(inf), "surface::slice_pitch()==surface::total_size() failed");
	}

	inf.mip_layout = surface::mip_layout::below;
	uint SlicePitchesVertical[15] = { 0x00180000, 0x00280000, 0x00380000, 0x00500000, 0x00600000, 0x00780000, 0x00880000, 0x00a00000, 0x00b00000, 0x00c80000, 0x00d80000, 0x00f00000, 0x01000000, 0x01180000, 0x01280000 };
	for (uint depthLevel = 0; depthLevel < countof(SlicePitches); depthLevel++)
	{
		inf.dimensions.z = (int)depthLevel + 1;
		oCHECK(surface::slice_pitch(inf) == SlicePitchesVertical[depthLevel], "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == surface::total_size(inf), "surface::slice_pitch()==surface::total_size() failed");
	}

	inf.mip_layout = surface::mip_layout::right;
	uint SlicePitchesHorizontal[15] = { 0x00180000, 0x00300000, 0x00480000, 0x00600000, 0x00780000, 0x00900000, 0x00a80000, 0x00c00000, 0x00d80000, 0x00f00000, 0x01080000, 0x01200000, 0x01380000, 0x01500000, 0x01680000 };
	for (uint depthLevel = 0; depthLevel < countof(SlicePitchesHorizontal); depthLevel++)
	{
		inf.dimensions.z = (int)depthLevel + 1;
		oCHECK(surface::slice_pitch(inf) == SlicePitchesHorizontal[depthLevel], "surface::slice_pitch() failed");
		oCHECK(surface::slice_pitch(inf) == surface::total_size(inf), "surface::slice_pitch()==surface::total_size() failed");
	}
}

oTEST(oSurface_surface)
{
	for (int i=1; i <= 16; ++i)
	{
		TESTsurface_row_pitch(i, 1);
		TESTsurface_row_pitch(1, i);
		TESTsurface_depth_pitch(i, 1);
		TESTsurface_depth_pitch(1, i);
		TESTsurface_slice_pitch(i);
	}

	surface::info_t inf;
	inf.dimensions = uint3(512,512,511);
	inf.format = surface::format::r8g8b8a8_unorm;
	inf.mip_layout = surface::mip_layout::tight;
	oCHECK(surface::offset(inf, 1)==0x1ff00000, "");

	uint3 mip_dimensions[] = 
	{ 
		uint3(1,1,1), 
		uint3(2,2,2), uint3(1,2,2), uint3(2,1,2), uint3(2,2,1), uint3(1,1,2), uint3(1,2,1), uint3(2,1,1), 
		uint3(3,3,3), uint3(1,3,3), uint3(3,1,3), uint3(3,3,1), uint3(1,1,3), uint3(1,3,1), uint3(3,1,1), 
		uint3(4,4,4),
		uint3(16,16,16),
		uint3(1024,1024,1024), uint3(1,1024,1024), uint3(1024,1,1024), uint3(1024,1024,1), uint3(1,1,1024), uint3(1,1024,1), uint3(1,1,1024),
	};
	uint NumMips[] = 
	{ 
		1, 
		2, 2, 2, 2, 2, 2, 2, 
		2, 2, 2, 2, 2, 2, 2, 
		3,
		5,
		11, 11, 11, 11, 11, 11, 11
	};

	for (auto i = 0; i < countof(NumMips); i++)
		oCHECK(surface::num_mips(surface::mip_layout::tight, mip_dimensions[i]) == NumMips[i], "surface::num_mips(.., int3(%d,%d,%d))==%d failed", mip_dimensions[i].x, mip_dimensions[i].y, mip_dimensions[i].z, NumMips[i]);
}
