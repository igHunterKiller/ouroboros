// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/projection.h>

using namespace ouro;

static const float kAspectRatio = 1.0f;
static const float kFovY = oDEFAULT_FOVY_RADIANSf;
static const float kFovX = kFovY;
static void check_persp(const char* name, const float4x4& persp)
{
	float2 fov = proj_fov(persp);
	oCHECK(equal(fov.x, kFovX), "%s fovx failed %f != %f", name, fov.x, kFovX);
	oCHECK(equal(fov.y, kFovY), "%s fovy failed %f != %f", name, fov.y, kFovY);

	float near = proj_near(persp);
	oCHECK(equal(near, oDEFAULT_NEAR_CLIPf), "%s near failed %f != %f", name, near, oDEFAULT_NEAR_CLIPf);

	// Precision really breaks down here.
	float far = proj_far(persp);
	oCHECK(abs(far - oDEFAULT_FAR_CLIPf) < 0.08f, "%s far failed %f != %f", name, far, oDEFAULT_FAR_CLIPf);

	float aspect_ratio = proj_aspect_ratio(persp);
	oCHECK(equal(aspect_ratio, kAspectRatio), "%s aspect ratio failed %f != %f", name, aspect_ratio, kAspectRatio);
}

oTEST(oMath_perspective)
{
	float4x4 persp = proj_fovy_lh(kFovY, kAspectRatio);
	check_persp("fovy_lh", persp);
	persp = proj_fovx_lh(kFovX, kAspectRatio);
	check_persp("fovx_lh", persp);
	persp = proj_fovy_rh(kFovY, kAspectRatio);
	check_persp("fovy_rh", persp);
	persp = proj_fovx_rh(kFovX, kAspectRatio);
	check_persp("fovx_rh", persp);
	persp = ortho_lh(2.0f, 2.0f);
	check_persp("ortho_lh", persp);
	persp = ortho_rh(2.0f, 2.0f);
	check_persp("ortho_rh", persp);
}
