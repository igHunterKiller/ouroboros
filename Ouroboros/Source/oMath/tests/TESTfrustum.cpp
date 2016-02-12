// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/cube_topology.h>
#include <oMath/hlslx.h>
#include <oMath/matrix.h>
#include <oMath/projection.h>
#include <oMath/equal.h>
#include <array>

using namespace ouro;

oTEST(oMath_frustum)
{
	static const frustum_t EXPECTED_LH =
	{
		float4( 1.0f,  0.0f,  0.0f,  1.0f),
		float4(-1.0f,  0.0f,  0.0f,  1.0f),
		float4( 0.0f, -1.0f,  0.0f,  1.0f),
		float4( 0.0f,  1.0f,  0.0f,  1.0f),
		float4( 0.0f,  0.0f,  1.0f,  1.0f),
		float4( 0.0f,  0.0f, -1.0f,  1.0f)
	};

	static const frustum_t EXPECTED_RH =
	{
		float4( 1.0f,  0.0f,  0.0f,  1.0f),
		float4(-1.0f,  0.0f,  0.0f,  1.0f),
		float4( 0.0f, -1.0f,  0.0f,  1.0f),
		float4( 0.0f,  1.0f,  0.0f,  1.0f),
		float4( 0.0f,  0.0f, -1.0f,  1.0f),
		float4( 0.0f,  0.0f,  1.0f,  1.0f)
	};

	static const frustum_t EXPECTED_PERSPECTIVE_LH = 
	{
		float4( 0.707f,    0.0f,  0.707f,   0.0f),
		float4(-0.707f,    0.0f,  0.707f,   0.0f),
		float4(   0.0f, -0.707f,  0.707f,   0.0f),
		float4(   0.0f,  0.707f,  0.707f,   0.0f),
		float4(   0.0f,    0.0f,  1.0f,    -0.1f),
		float4(   0.0f,    0.0f, -1.0f,   100.0f)
	};

	static const frustum_t EXPECTED_PERSPECTIVE_RH =
	{
		float4( 0.707f,    0.0f, -0.707f,   0.0f),
		float4(-0.707f,    0.0f, -0.707f,   0.0f),
		float4(   0.0f, -0.707f, -0.707f,   0.0f),
		float4(   0.0f,  0.707f, -0.707f,   0.0f),
		float4(   0.0f,    0.0f, -1.0f,    -0.1f),
		float4(   0.0f,    0.0f,  1.0f,   100.0f)
	};

	const unsigned int MAX_ULPS = 1800;

	frustum_t f;
	float4x4 P = ortho_offcenter_lh(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
	f = proj_frustum(P, false);
	oCHECK(equal(EXPECTED_LH, f, MAX_ULPS), "orthographic LH comparison failed");

	P = ortho_offcenter_rh(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
	f = proj_frustum(P, false);
	oCHECK(equal(EXPECTED_RH, f, MAX_ULPS), "orthographic RH comparison failed");

	P = proj_fovy_lh(radians(90.0f), 1.0f, 0.1f, 100.0f);
	f = proj_frustum(P, true);
	oCHECK(equal(EXPECTED_PERSPECTIVE_LH, f, MAX_ULPS), "perspective LH comparison failed");
	
	P = proj_fovy_rh(radians(90.0f), 1.0f, 0.1f, 100.0f);
	f = proj_frustum(P, true);
	oCHECK(equal(EXPECTED_PERSPECTIVE_RH, f, MAX_ULPS), "perspective RH comparison failed");
}

oTEST(oMath_frustum_calc_corners)
{
	static const float3 EXPECTED_ORTHO_LH[8] =
	{
		float3(-1.0f, 1.0f, -1.0f),
		float3(-1.0f, 1.0f, 1.0f),
		float3(-1.0f, -1.0f, -1.0f),
		float3(-1.0f, -1.0f, 1.0f),
		float3(1.0f, 1.0f, -1.0f),
		float3(1.0f, 1.0f, 1.0f),
		float3(1.0f, -1.0f, -1.0f),
		float3(1.0f, -1.0f, 1.0f),
	};

	static const float3 EXPECTED_VP_PROJ_LH[8] =
	{
		float3(9.9f, 0.1f, -10.1f),
		float3(-90.0f, 100.0f, -110.0f),
		float3(9.9f, -0.1f, -10.1f),
		float3(-90.0f, -100.0f, -110.0f),
		float3(9.9f, 0.1f, -9.9f),
		float3(-90.0f, 100.0f, 90.0f),
		float3(9.9f, -0.1f, -9.9f),
		float3(-90.0f, -100.0f, 90.0f),
	};

	const int MAX_ULPS = 1800;

	float4x4 P = ortho_offcenter_lh(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
	oCHECK(!has_projection(P), "Ortho matrix says it has perspective when it doesn't");
	frustum_t f = proj_frustum(P);
	float3 corners[8];
	oCHECK(proj_corners(f, corners), "oExtractFrustumCorners failed (f)");
	for (auto i = 0; i < 8; i++)
		oCHECK(equal(EXPECTED_ORTHO_LH[i], corners[i], MAX_ULPS), "corner %d mismatch", i);

	P = proj_fovy_lh(radians(90.0f), 1.0f, 0.1f, 100.0f);
	float4x4 V = lookat_lh(float3(10.0f, 0.0f, -10.0f), float3(0.0f, 0.0f, -10.0f), float3(0.0f, 1.0f, 0.0f));
	float4x4 VP = V * P;
	frustum_t wsf = proj_frustum(VP);
	oCHECK(proj_corners(wsf, corners), "oExtractFrustumCorners failed (wsf)");

	float nearWidth = distance(corners[oCUBE_RIGHT_TOP_NEAR], corners[oCUBE_LEFT_TOP_NEAR]);
	float farWidth = distance(corners[oCUBE_RIGHT_TOP_FAR], corners[oCUBE_LEFT_TOP_FAR]);
	oCHECK(equal(0.2f, nearWidth, MAX_ULPS), "width of near plane incorrect");
	oCHECK(equal(200.0f, farWidth, MAX_ULPS), "width of far plane incorrect");
	for (auto i = 0; i < 8; i++)
		oCHECK(equal(EXPECTED_VP_PROJ_LH[i], corners[i], MAX_ULPS), "corner %d mismatch", i);

	// Verify world pos extraction math.  This is used in deferred rendering.
	const float3 TestPos = float3(-30, 25, 41);
	float3 ComputePos;
	{
		frustum_t psf = proj_frustum(P);
		float3 EyePos = view_eye(V);
		float EyeZ = mul(V, float4(TestPos, 1.0f)).z;
		float InverseFarPlaneDistance = 1.0f / distance(wsf[oCUBE_FAR], EyePos);
		float Depth = EyeZ * InverseFarPlaneDistance;

		float4 ProjTest = mul(VP, float4(TestPos, 1.0f));
		ProjTest /= ProjTest.w;

		float2 LerpFact = ProjTest.xy() * 0.5f + 0.5f;
		LerpFact.y = 1.0f - LerpFact.y;

		frustum_t Frust = proj_frustum(P);
		oCHECK(proj_corners(Frust, corners), "oExtractFrustumCorners failed (Frust)");
		float3x3 FrustTrans = (float3x3)invert(V);
		float3 LBF = mul(FrustTrans, corners[oCUBE_LEFT_BOTTOM_FAR]);
		float3 RBF = mul(FrustTrans, corners[oCUBE_RIGHT_BOTTOM_FAR]);
		float3 LTF = mul(FrustTrans, corners[oCUBE_LEFT_TOP_FAR]);
		float3 RTF = mul(FrustTrans, corners[oCUBE_RIGHT_TOP_FAR]);

		float3 FarPointTop = lerp(LTF,  RTF, LerpFact.x);
		float3 FarPointBottom = lerp(LBF, RBF, LerpFact.x);
		float3 FarPoint = lerp(FarPointTop, FarPointBottom, LerpFact.y);

		ComputePos = EyePos + (FarPoint * Depth);
	}
	oCHECK(equal(ComputePos, TestPos, MAX_ULPS), "Computed world pos is incorrect");
}

