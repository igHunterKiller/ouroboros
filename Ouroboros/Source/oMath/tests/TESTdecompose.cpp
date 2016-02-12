// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/matrix.h>

using namespace ouro;

oTEST(oMath_decompose)
{
	float3 test_rot_degrees = float3(45.0f, 32.0f, 90.0f);
	float3 test_tx = float3(-14.0f, 70.0f, 32.0f);
	float3 test_scale = float3(17.0f, 2.0f, 3.0f); // negative scale not supported

	// Remember assembly of the matrix must be in the canonical order for the 
	// same values to come back out through decomposition.
	float4x4 m = scale(test_scale) * rotate(radians(test_rot_degrees)) * translate(test_tx);

	float3 scale, sh, rot, tx; float4 persp;
	decompose(m, &scale, &sh, &rot, &tx, &persp);

	rot = degrees(rot);
	oCHECK(equal(test_scale, scale), "Scales are not the same through assembly and decomposition");
	oCHECK(equal(test_rot_degrees, rot), "Rotations are not the same through assembly and decomposition");
	oCHECK(equal(test_tx, tx), "Translations are not the same through assembly and decomposition");
}

