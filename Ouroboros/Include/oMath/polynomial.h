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
	A = c / d;
	B = b / d;
	C = a / d;

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
			//float u = cbrt(-q);
			float u = powf(-q, 1.0f/3.0f);
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
		//float u = cbrt(sqrt_D + fabsf(q));
		float u = powf(sqrt_D + fabsf(q), 1.0f/3.0f);

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

	A = d / e;
	B = c / e;
	C = b / e;
	D = a / e;

	// subsitute x = y - A / 4 to eliminate the cubic term: x^4 + px^2 + qx + r = 0

	sq_A = A * A;
	p = -3.0f / 8.0f * sq_A + B;
	q = 1.0f / 8.0f * sq_A * A - 1.0f / 2.0f * A * B + C;
	r = -3.0f / 256.0f * sq_A * sq_A + 1.0f / 16.0f * sq_A * B - 1.0f / 4.0f * A * C + D;

	if (polynomial_float_is_zero(r))
	{
		// no absolute term:y(y ^ 3 + py + q) = 0
		num = cubic(q, p, 0.0f, 1.0f, out_roots);
		out_roots[num++] = 0;
	}
	else
	{
		// solve the resolvent cubic...
		cubic(1.0f / 2.0f * r * p - 1.0f / 8.0f * q * q, -r, -1.0f / 2.0f * p, 1.0f, out_roots);

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
