// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Polynomial root solvers. Each returns the number of real roots.

// Original author of cube, quadric and quartic:
// Doué, Jean-François, C++ Vector and Matrix Algebra Routines, 
// Graphics Gems IV, p. 534-557, code: p. 535-557, vec_mat/.
// http://tog.acm.org/resources/GraphicsGems/gemsiv/vec_mat/ray/solver.c

#pragma once
#include <cmath>

namespace ouro {

inline bool polynomial_float_is_zero(float f) { return fabsf(f) < 1e-15; }

// ax^2 + bx + c = 0
//
//             /---------
//      -b +- V b^2 - 4ac
// x = -------------------
//             2a
inline int quadratic(float a, float b, float c, float out_roots[2])
{
	const float q = (b * b) - (4.0f * a * c);
	if (q >= 0.0f && !polynomial_float_is_zero(a))
	{
		const float sq = sqrtf(q);
		const float d = 1.0f / (2.0f * a);
		out_roots[0] = (-b + sq) * d;
		out_roots[1] = (-b - sq) * d;
		return 2;
	}

	return 0;
}

inline int cubic(float a, float b, float c, float d, float out_roots[3])
{
	int	num;
	float sub,
	A, B, C,
	sq_A, p, q,
	cb_p, D;

	// normalize the equation:x ^ 3 + Ax ^ 2 + Bx  + C = 0
	A = b / a;
	B = d / a;
	C = c / a;

	// substitute x = y - A / 3 to eliminate the quadric term: x^3 + px + q = 0

	sq_A = A * A;
	p = 1.0f/3.0f * (-1.0f/3.0f * sq_A + B);
	q = 1.0f/2.0f * (2.0f/27.0f * A *sq_A - 1.0f/3.0f * A * B + C);

	// use Cardano's formula

	cb_p = p * p * p;
	D = q * q + cb_p;

	if (polynomial_float_is_zero(D))
	{
		if (polynomial_float_is_zero(q))
		{
			// one triple solution
			out_roots[0] = 0.0f;
			num = 1;
		}
		else
		{
			// one single and one double solution
			float u = cbrtf(-q);
			out_roots[0] = 2.0f * u;
			out_roots[1] = - u;
			num = 2;
		}
	}
	else if (D < 0.0f)
	{
		// casus irreductibilis: three real solutions
		float phi = 1.0f/3.0f * acosf(-q / sqrtf(-cb_p));
		float t = 2.0f * sqrtf(-p);
		out_roots[0] = t * cosf(phi);

		const float PI = 3.14159265358979323846f;

		out_roots[1] = -t * cosf(phi + PI / 3.0f);
		out_roots[2] = -t * cosf(phi - PI / 3.0f);
		num = 3;
	}
	else
	{
		// one real solution
		float sqrt_D = sqrtf(D);
		float u = cbrtf(sqrt_D + fabsf(q));

		if (q > 0.0f)
			out_roots[0] = - u + p / u ;
		else
			out_roots[0] = u - p / u;
		num = 1;
	}

	// resubstitute
	sub = 1.0f / 3.0f * A;
	for (int i = 0; i < num; i++)
		out_roots[i] -= sub;
	return num;
}

inline int cubic2(float a, float b, float c, float d, float out_roots[3])
{
	// Another method.

	// http://www.1728.org/cubic2.htm

	const float sqrt3  = 1.7320508075688772935274463415059f;
	const float inv_27 = 1.0f / 27.0f;
	const float inv_a  = 1.0f / a;
	const float inv_a2 = inv_a * inv_a;
	const float inv_a3 = inv_a * inv_a2;
	const float b2     = b * b;
	const float b3     = b * b2;
	const float b3a    = b * (1.0f/3.0f) * inv_a;

	const float f      = ((3.0f * c * inv_a) - (b2 * inv_a2)) / 3.0f;
	const float g      = ((2.0f * b3 * inv_a3) - (9.0f * b * c * inv_a2) + (27.0f * d * inv_a)) * inv_27;
	const float g2     = g * g;
	const float h      = (g2 * 0.25f) + (f * f * f * inv_27);
	const float i2     = (g2 * 0.25f) - h;

	const float i      = sqrtf(i2);

	if (h > 0)
	{
		// 1 real root

		const float neg_half_g = g * -0.5f;
		const float sqrt_h = sqrtf(h);

		const float r = neg_half_g + sqrt_h;
		const float s = cbrtf(r);
		const float t = neg_half_g - sqrt_h;
		const float u = cbrtf(t);

		const float su = s + u;
		
		if (i2 < 0.0f)
		{
			out_roots[0] = su - b3a;
			return 1;
		}
		
		const float neg_half_su = su * -0.5f;
		const float neg_half_su_minus_b3a = neg_half_su - b3a;
		const float v = i * (s - u) * sqrt3 * 0.5f;

		out_roots[1] = neg_half_su_minus_b3a + v;
		out_roots[2] = neg_half_su_minus_b3a - v;

		return 3;
	}

	else if (equal(h, 0.0f) && equal(g, 0.0f) && equal(h, 0.0f))
	{
		// all roots real and equal
		const float w = -cbrtf(d * inv_a);
		out_roots[0] = w;
		out_roots[1] = w;
		out_roots[2] = w;

		return 1;
	}
	
	else if (h <= 0)
	{
		// all real

		const float j = cbrtf(i);
		const float k = acosf(-(g / (2.0f * i)));
		const float l = -j;
		const float k_div3 = k / 3.0f;
		float sin_k_div3, cos_k_div3;
		sincos(k_div3, sin_k_div3, cos_k_div3);
		const float m = cos_k_div3;
		const float n = sqrt3 * sin_k_div3;
		const float p = -b3a;
		
		out_roots[0] = (2.0f * j * cos_k_div3) - b3a;
		out_roots[1] = l * (m + n) + p;
		out_roots[2] = l * (m - n) + p;

		return 3;
	}

	return 0;
}

inline int quadric(float a, float b, float c, float out_roots[2])
{
	float p, q, D;
	
	// make sure we have a d2 equation
	if (polynomial_float_is_zero(c))
	{
		if (polynomial_float_is_zero(b))
			return 0;

		out_roots[0] = -a / b;
		return 1;
	}

	// normal for: x^2 + px + q
	p = b / (2.0f * c);
	q = a / c;
	D = p * p - q;

	if (polynomial_float_is_zero(D))
	{
		// one double root
		out_roots[0] = out_roots[1] = -p;
		return 1;
	}

	if (D < 0.0f)
		return 0; // no real root

	else
	{
		// two real roots
		float sqrt_D = sqrtf(D);
		out_roots[0] = sqrt_D - p;
		out_roots[1] = -sqrt_D - p;
		return 2;
	}
}

inline int quartic(float a, float b, float c, float d, float e, float out_roots[4])
{
	float z, u, v, sub,
	A, B, C, D,
	sq_A, p, q, r;
	int num;

	// normalize the equation:x ^ 4 + Ax ^ 3 + Bx ^ 2 + Cx + D = 0

	A = b / a;
	B = c / a;
	C = d / a;
	D = e / a;

	// subsitute x = y - A / 4 to eliminate the cubic term: x^4 + px^2 + qx + r = 0

	sq_A = A * A;
	p = -3.0f / 8.0f * sq_A + B;
	q = 1.0f / 8.0f * sq_A * A - 1.0f / 2.0f * A * B + C;
	r = -3.0f / 256.0f * sq_A * sq_A + 1.0f / 16.0f * sq_A * B - 1.0f / 4.0f * A * C + D;

	if (polynomial_float_is_zero(r))
	{
		// no absolute term:y(y ^ 3 + py + q) = 0
		num = cubic(1.0f, 0.0f, p, q, out_roots);
		out_roots[num++] = 0;
	}
	else
	{
		// solve the resolvent cubic...
		cubic(1.0f, -1.0f / 2.0f * p, -r, 1.0f / 2.0f * r * p - 1.0f / 8.0f * q * q, out_roots);

		// ...and take the one real solution...
		z = out_roots[0];

		// ...to build two quadratic equations
		u = z * z - r;
		v = 2.0f * z - p;

		if (polynomial_float_is_zero(u))
			u = 0.0f;
		else if (u > 0.0f)
			u = sqrtf(u);
		else
			return 0;

		if (polynomial_float_is_zero(v))
			v = 0.0f;
		else if (v > 0.0f)
			v = sqrtf(v);
		else
			return 0;

		num = quadric(z - u, q < 0 ? -v : v, 1.0f, out_roots);
		num += quadric(z + u, q < 0 ? v : -v, 1.0f, out_roots + num);
	}

	// resubstitute
	sub = 1.0f / 4.0f * A;
	for (int i = 0; i < num; i++)
		out_roots[i] -= sub;

	return num;
}

}
