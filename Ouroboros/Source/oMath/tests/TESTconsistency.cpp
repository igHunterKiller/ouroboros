// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/pov.h>
#include <oMath/projection.h>
#include <oMath/matrix.h>
#include <DirectXMath.h>

using namespace ouro;

oTEST(oMath_consistency)
{
	float3 pos = float3              (19.5f, 1.0f, 91.0f);
	auto DXpos = DirectX::XMVectorSet(19.5f, 1.0f, 91.0f, 1.0f);

	{
		float4x4 proj = proj_fovy_lh                     (radians(60.0f), 1.0f, 0.1f, 1000.0f);
		auto   DXproj = DirectX::XMMatrixPerspectiveFovLH(radians(60.0f), 1.0f, 0.1f, 1000.0f);

		auto DXprojection = (const float4x4&)DXproj;

		oCHECK(equal(proj[0], DXprojection[0]), "projection matrices not equal 0");
		oCHECK(equal(proj[1], DXprojection[1]), "projection matrices not equal 1");
		oCHECK(equal(proj[2], DXprojection[2]), "projection matrices not equal 2");
		oCHECK(equal(proj[3], DXprojection[3]), "projection matrices not equal 3");
	}

	{
		float4x4 proj = proj_fovy_rh                     (radians(60.0f), 1.0f, 0.1f, 1000.0f);
		auto   DXproj = DirectX::XMMatrixPerspectiveFovRH(radians(60.0f), 1.0f, 0.1f, 1000.0f);

		auto DXprojection = (const float4x4&)DXproj;

		oCHECK(equal(proj[0], DXprojection[0]), "projection matrices not equal 0");
		oCHECK(equal(proj[1], DXprojection[1]), "projection matrices not equal 1");
		oCHECK(equal(proj[2], DXprojection[2]), "projection matrices not equal 2");
		oCHECK(equal(proj[3], DXprojection[3]), "projection matrices not equal 3");
	}

	{
		float4x4 proj = ortho_lh(2.0f, 2.0f, 0.1f, 1000.0f);
		auto   DXproj = DirectX::XMMatrixOrthographicLH(2.0f, 2.0f, 0.1f, 1000.0f);

		auto DXprojection = (const float4x4&)DXproj;

		oCHECK(equal(proj[0], DXprojection[0]), "projection matrices not equal 0");
		oCHECK(equal(proj[1], DXprojection[1]), "projection matrices not equal 1");
		oCHECK(equal(proj[2], DXprojection[2]), "projection matrices not equal 2");
		oCHECK(equal(proj[3], DXprojection[3]), "projection matrices not equal 3");
	}

	{
		float4x4 proj = ortho_rh(2.0f, 2.0f, 0.1f, 1000.0f);
		auto   DXproj = DirectX::XMMatrixOrthographicRH(2.0f, 2.0f, 0.1f, 1000.0f);

		auto DXprojection = (const float4x4&)DXproj;

		oCHECK(equal(proj[0], DXprojection[0]), "projection matrices not equal 0");
		oCHECK(equal(proj[1], DXprojection[1]), "projection matrices not equal 1");
		oCHECK(equal(proj[2], DXprojection[2]), "projection matrices not equal 2");
		oCHECK(equal(proj[3], DXprojection[3]), "projection matrices not equal 3");
	}
	
	float4x4 proj = proj_fovy_lh                     (radians(60.0f), 1.0f, 0.1f, 1000.0f);
	auto   DXproj = DirectX::XMMatrixPerspectiveFovLH(radians(60.0f), 1.0f, 0.1f, 1000.0f);

	auto   eye = float3              (0.0f, 0.0f, -5.0f);
	auto DXeye = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);

	auto   at  = float3              (0.0f, 0.0f,  0.0f);
	auto DXat  = DirectX::XMVectorSet(0.0f, 0.0f,  0.0f, 1.0f);

	auto   up  = float3              (0.0f, 1.0f,  0.0f);
	auto DXup  = DirectX::XMVectorSet(0.0f, 1.0f,  0.0f, 1.0f);

	float4x4 view  = lookat_lh                  (eye,   at,   up);
	auto   DXview  = DirectX::XMMatrixLookAtLH(DXeye, DXat, DXup);
	auto   DXview2 = (const float4x4&)DXview;

	oCHECK(equal(view[0], DXview2[0]), "view matrices not equal 0");
	oCHECK(equal(view[1], DXview2[1]), "view matrices not equal 1");
	oCHECK(equal(view[2], DXview2[2]), "view matrices not equal 2");
	oCHECK(equal(view[3], DXview2[3]), "view matrices not equal 3");

	auto   vp  = view * proj;
	auto DXvp  = DirectX::XMMatrixMultiply(DXview, DXproj);
	auto DXvp2 = (const float4x4&)DXvp;

	oCHECK(equal(vp[0], DXvp2[0]), "view projection matrices not equal 0");
	oCHECK(equal(vp[1], DXvp2[1]), "view projection matrices not equal 1");
	oCHECK(equal(vp[2], DXvp2[2]), "view projection matrices not equal 2");
	oCHECK(equal(vp[3], DXvp2[3]), "view projection matrices not equal 3");

	{
		float3 WSpos(0.0f, 0.0f, 100.0f);

		pov_t pov(uint2(100, 100), radians(60.0f), 0.1f, 1000.0f);
		pov.view(view);

		auto VPpos = pov.world_to_viewport(WSpos);

		auto WSpos2 = pov.viewport_to_world(VPpos);

		oCHECK(equal(WSpos, WSpos2), "pos tx and back isn't equal");
	}

	{
		float4x4 proj2 = proj_fovy_lh(radians(60.0f), 1.0f, 0.1f, 1000.0f);
		float4x4 proj_inv = proj_inv_fovy_lh(radians(60.0f), 1.0f, 0.1f, 1000.0f);
		float4x4 test_proj_inv = invert(proj2);

		oCHECK(equal(test_proj_inv, proj_inv), "fused proj + inv calc not the same as inverting proj lh");
	}

	{
		float4x4 proj2 = proj_fovy_rh(radians(60.0f), 1.0f, 0.1f, 1000.0f);
		float4x4 proj_inv = proj_inv_fovy_rh(radians(60.0f), 1.0f, 0.1f, 1000.0f);
		float4x4 test_proj_inv = invert(proj2);

		oCHECK(equal(test_proj_inv, proj_inv), "fused proj + inv calc not the same as inverting proj rh");
	}
}
