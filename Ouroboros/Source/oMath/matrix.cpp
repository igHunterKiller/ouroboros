// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/matrix.h>
#include <oMath/hlslx.h>
#include <oMath/equal.h>

#include <vectormath/scalar/cpp/vectormath_aos.h>
#include <vectormath/scalar/cpp/vectormath_aos_d.h>
using namespace Vectormath::Aos;

namespace ouro {

float3x3 invert(const float3x3& m)
{
	Matrix3 mtx = inverse((const Matrix3&)m);
	return (float3x3&)mtx;
}

float4x4 invert(const float4x4& m)
{
	Matrix4 mtx = inverse((const Matrix4&)m);
	return (float4x4&)mtx;
}

static float3 scale(const float3& a, float new_len)
{
	// returns the specified vector with the specified length
	float oldLength = length(a);
	if (equal(oldLength, 0.0f))
		return a;
	return a * (new_len / oldLength);
}

static const float3 combine(const float3& a, const float3& b, float aScale, float bScale)
{
  return a * aScale + b * bScale;
}

bool decompose(const float4x4& m, float3* out_scale, float3* out_shear, float3* out_rotation, float3* out_translation, float4* out_perspective)
{
  // Spencer W. Thomas, assumed public domain
  // http://tog.acm.org/resources/GraphicsGems/gemsii/unmatrix.c

//int
//unmatrix(mat, tran)
//Matrix4 *mat;
//double tran[16];
//{
 	register int i, j;
 	float4x4 locmat;
 	float4x4 pmat, invpmat, tinvpmat;
 	/* Vector4 type and functions need to be added to the common set. */
 	float4 prhs, psol;
 	float3 row[3], pdum3;

 	locmat = m;
 	/* Normalize the matrix. */
 	if (locmat[3][3] == 0)
 		return 0;
 	for (i=0; i<4;i++)
 		for (j=0; j<4; j++)
 			locmat[i][j] /= locmat[3][3];
 	/* pmat is used to solve for perspective, but it also provides
 	 * an easy way to test for singularity of the upper 3x3 component.
 	 */
 	pmat = locmat;
 	for (i=0; i<3; i++)
 		pmat[i][3] = 0;
 	pmat[3][3] = 1;

 	if (determinant(pmat) == 0.0)
 		return 0;

 	/* First, isolate perspective.  This is the messiest. */
 	if (locmat[0][3] != 0 || locmat[1][3] != 0 ||
 		locmat[2][3] != 0) {
 		/* prhs is the right hand side of the equation. */
 		prhs.x = locmat[0][3];
 		prhs.y = locmat[1][3];
 		prhs.z = locmat[2][3];
 		prhs.w = locmat[3][3];

 		/* Solve the equation by inverting pmat and multiplying
 		 * prhs by the inverse.  (This is the easiest way, not
 		 * necessarily the best.)
 		 * inverse function (and det4x4, above) from the Matrix
 		 * Inversion gem in the first volume.
 		 */
 		invpmat = invert(pmat);
		tinvpmat = transpose(invpmat);
		psol = mul(tinvpmat, prhs);
 
 		/* Stuff the answer away. */
		
 		out_perspective->x = psol.x;
 		out_perspective->y = psol.y;
 		out_perspective->z = psol.z;
 		out_perspective->w = psol.w;
 		/* Clear the perspective partition. */
 		locmat[0][3] = locmat[1][3] =
 			locmat[2][3] = 0;
 		locmat[3][3] = 1;
 	} else		/* No perspective. */
 		out_perspective->x = out_perspective->y = out_perspective->z =
 			out_perspective->w = 0;

 	/* Next take care of translation (easy). */
 	for (i=0; i<3; i++) {
 		(*out_translation)[i] = locmat[3][i];
 		locmat[3][i] = 0;
 	}

 	/* Now get scale and shear. */
 	for (i=0; i<3; i++) {
 		row[i].x = locmat[i][0];
 		row[i].y = locmat[i][1];
 		row[i].z = locmat[i][2];
 	}

 	/* Compute X scale factor and normalize first row. */
 	out_scale->x = length(*(float3*)&row[0]);
 	*(float3*)&row[0] = scale(*(float3*)&row[0], float(1.0));

	/* Compute XY shear factor and make 2nd row orthogonal to 1st. */
	out_shear->x = dot(*(float3*)&row[0], *(float3*)&row[1]);
	*(float3*)&row[1] = combine(*(float3*)&row[1], *(float3*)&row[0], float(1.0), -out_shear->x);

 	/* Now, compute Y scale and normalize 2nd row. */
 	out_scale->y = length(*(float3*)&row[1]);
 	*(float3*)&row[1] = scale(*(float3*)&row[1], float(1.0));
 	out_shear->x /= out_scale->y;

 	/* Compute XZ and YZ shears, orthogonalize 3rd row. */
 	out_shear->y = dot(*(float3*)&row[0], *(float3*)&row[2]);
	*(float3*)&row[2] = combine(*(float3*)&row[2], *(float3*)&row[0], float(1.0), -out_shear->y);
 	out_shear->z = dot(*(float3*)&row[1], *(float3*)&row[2]);
 	*(float3*)&row[2] = combine(*(float3*)&row[2], *(float3*)&row[1], float(1.0), -out_shear->z);

 	/* Next, get Z scale and normalize 3rd row. */
 	out_scale->z = length(*(float3*)&row[2]);
 	*(float3*)&row[2] = scale(*(float3*)&row[2], float(1.0));
 	out_shear->y /= out_scale->z;
 	out_shear->z /= out_scale->z;
  
 	/* At this point, the matrix (in rows[]) is orthonormal.
 	 * Check for a coordinate system flip.  If the determinant
 	 * is -1, then negate the matrix and the scaling factors.
 	 */
 	if (dot(*(float3*)&row[0], cross(*(float3*)&row[1], *(float3*)&row[2])) < 0.0f)
 		for (i = 0; i < 3; i++) {
 			(*out_scale)[i] *= -1.0f;
 			row[i].x *= -1.0f;
 			row[i].y *= -1.0f;
 			row[i].z *= -1.0f;
 		}
 
 	/* Now, get the rotations out, as described in the gem. */
 	out_rotation->y = asin(-row[0].z);
 	if (cos(out_rotation->y) != 0.0f) {
 		out_rotation->x = atan2(row[1].z, row[2].z);
 		out_rotation->z = atan2(row[0].y, row[0].x);
 	} else {
 		out_rotation->x = atan2(-row[2].x, row[1].y);
 		out_rotation->z = 0.0f;
 	}
 	/* All done! */
 	return 1;
}

float3x3 orthonormalize(const float3x3& m)
{
	// NOTE: I don't think this is correct... ortho_normalizing an identity matrix does not yield identity (n[2].z is sign-flipped)
	// This is described as for a right-handed matrix. Can I just flip Z and be done with it for a left-hander? Does that even 
	// make sense for a generic affine matrix?
	if (1)
		throw std::invalid_argument("unreliable implementation");

	// modified Gram-Schmidt
	float3x3 n;
	n[0] = normalize(m[0]);
	n[1] = normalize(m[1] - dot(m[1], m[0]) * m[0]);
	n[2] = n[2] - dot(n[2], n[0]) * n[0];
	n[2] -= dot(n[2], n[1]) * n[1];
	n[2] = normalize(n[2]);
	return n;
}

float4x4 normalization(const float3& aabox_min, const float3& aabox_max)
{
	const float3 s = max(abs(aabox_max - aabox_min));
	// Scaling the bounds can introduce floating point precision issues near the 
	// bounds therefore add epsilon to the translation.
	const float3 t = (-aabox_min / s) + float3(FLT_EPSILON);
	return mul(scale(rcp(s)), translate(t));
}

float4x4 remove_shear(const float4x4& m)
{
	float3 s, sh, r, t; float4 p;
	if (!decompose(m, &s, &sh, &r,&t, &p))
		throw std::invalid_argument("decompose failed");
	return scale(s) * rotate(r) * translate(t);
}

float4x4 remove_scale_shear(const float4x4& m)
{
	float3 s, sh, r, t; float4 p;
	if (!decompose(m, &s, &sh, &r,&t, &p))
		throw std::invalid_argument("decompose failed");

	return rotate(r) * translate(t);
}

float4x4 rotate(float radians, const float3& normalized_rotation_axis)
{
	Matrix4 m = Matrix4::rotation(radians, (const Vector3&)normalized_rotation_axis);
	return (float4x4&)m;
}

float4x4 rotate(const float3& normalized_from, const float3& normalized_to)
{
	float d = dot(normalized_from, normalized_to);

	// Check for very close to identity
	if (d > 0.99999f)
		return kIdentity4x4;

	// Check for very close to opposite
	if (d < -0.99999f)
		return float4x4(
			float4(-1.0f,  0.0f,  0.0f, 0.0f),
			float4( 0.0f, -1.0f,  0.0f, 0.0f),
			float4( 0.0f,  0.0f, -1.0f, 0.0f),
			float4( 0.0f,  0.0f,  0.0f, 1.0f));

	float a = acos(d / (length(normalized_from) * length(normalized_to)));
	float3 normalized_axis = normalize(cross(normalized_from, normalized_to));
	return rotate(a, normalized_axis);
}

float4x4 rotate(const float4& quaternion)
{
	Matrix4 m = Matrix4::rotation((const Quat&)quaternion);
	return (float4x4&)m;
}

float4 qrotate(const float3& radians)
{
	Quat q = Quat::Quat(Matrix3::rotationZYX((const Vector3&)radians));
	return (float4&)q;
}

float4 qrotate(float radians, const float3& normalized_rotation_axis)
{
	Quat q = Quat::rotation(radians, (const Vector3&)normalized_rotation_axis);
	return (float4&)q;
}

float4 qrotate(const float3& normalized_from, const float3& normalized_to)
{
	Quat q = Quat::rotation((const Vector3&)normalized_from, (const Vector3&)normalized_to);
	return (float4&)q;
}

float4 qrotate(const float4x4& m)
{
	Quat q = Quat(((const Matrix4&)m).getUpper3x3());
	return (float4&)q;
}

float4x4 rotatetranslate(const float3x3& rotation, const float3& translation)
{
	return float4x4(rotation, translation);
}

float4x4 rotate_xy_planar(const float4& normalized_plane)
{
	float4x4 m = rotate(kZAxis, normalized_plane.xyz());

	// Since rotate takes no default axis it will flip the world when the 
	// angle between the source and destination is 0. Since the desire is to force 
	// rotation around y negate the y portion of the rotation when the plane's 
	// normal is float3(0.0f, 0.0f, -1.0f)
	if (normalized_plane.z == -1.0f)
		m[1].y = -m[1].y;

	float3 offset(0.0f, 0.0f, normalized_plane.w);
	offset = mul((float3x3)m, offset);
	m[3].x = offset.x;
	m[3].y = offset.y;
	m[3].z = offset.z;
	return m;
}

float3 rotation(const float4x4& m)
{
	float3 s, sh, r, t; float4 p;
	if (!decompose(m, &s, &sh, &r,&t, &p))
		throw std::invalid_argument("decompose failed");
	return r;
}

float4x4 lookat_lh(const float3& eye, const float3& at, const float3& up)
{
	float3 z = normalize(at - eye);
	float3 x = normalize(cross(up, z));
	float3 y = cross(z, x);
	return float4x4(
    float4(         x.x,          y.x,            z.x, 0.0f),
    float4(         x.y,          y.y,            z.y, 0.0f),
    float4(         x.z,          y.z,            z.z, 0.0f),
    float4(-dot(x, eye), -dot(y, eye),   -dot(z, eye), 1.0f));
}

float4x4 lookat_rh(const float3& eye, const float3& at, const float3& up)
{
	float3 z = normalize(eye - at);
	float3 x = normalize(cross(up, z));
	float3 y = cross(z, x);
	return float4x4(
    float4(         x.x,          y.x,            z.x, 0.0f),
    float4(         x.y,          y.y,            z.y, 0.0f),
    float4(         x.z,          y.z,            z.z, 0.0f),
    float4(-dot(x, eye), -dot(y, eye),   -dot(z, eye), 1.0f));
}

float4x4 translate_view_to_fit(const float4x4& view, float fovy_radians, const float4& sphere)
{
	const float ratio_y  = tan(fovy_radians * 0.5f);
	const float offset   = (sphere.w) / ratio_y;
	float4x4 view_inv    = invert(view);
	const float3 new_pos = sphere.xyz() - view_inv[2].xyz() * offset;
	view_inv[3]          = float4(new_pos, view_inv[3].w);
	return invert(view_inv);
}

}