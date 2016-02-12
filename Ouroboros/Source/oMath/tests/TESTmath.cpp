// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/matrix.h>

oTEST(oMath_matrix_math)
{
	const int MAX_ULPS = 1800;
	float3 TestVec = normalize(float3(-54, 884, 32145));
	float3 TargetVecs[] =
	{
		TestVec, // Identity
		-TestVec, // Flip
		normalize(float3(TestVec.y, TestVec.z, TestVec.x)), // Axis swap
		normalize(float3(-242, 1234, 3)) // Other
	};

	int i = 0;
	for (const auto& t : TargetVecs)
	{
		float4x4 Rotation = rotate(TestVec, t);
		float3 RotatedVec = mul(Rotation, TestVec);
		oCHECK(equal(RotatedVec, t, MAX_ULPS), "rotate failed with #%d (%f %f %f)", i, t.x, t.y, t.z);
		i++;
	}
}
