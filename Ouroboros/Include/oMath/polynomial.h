// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Polynomial root solvers. Each returns the number of real roots.

// Original author of cube, quadric and quartic:
// Doué, Jean-François, C++ Vector and Matrix Algebra Routines, 
// Graphics Gems IV, p. 534-557, code: p. 535-557, vec_mat/.
// http://tog.acm.org/resources/GraphicsGems/gemsiv/vec_mat/ray/solver.c

#pragma once
#include <cmath>

namespace ouro {
	namespace detail {

template<typename T> T     fabs_(T f)     { return fabs(f); }
template<>    inline float fabs_(float f) { return fabsf(f); }

template<typename T> T     sqrt_(T f)     { return sqrt(f); }
template<>    inline float sqrt_(float f) { return sqrtf(f); }

template<typename T> T     cbrt_(T x)     { return cbrt(x); }
template<>    inline float cbrt_(float x) { return cbrtf(x); }

template<typename T> T     sin_(T x)      { return sin(x); }
template<>    inline float sin_(float x)  { return sinf(x); }

template<typename T> T     cos_(T x)      { return cos(x); }
template<>    inline float cos_(float x)  { return cosf(x); }

template<typename T> void sincos_(T x, T& out_sin, T& out_cos)             { sincos(x, out_sin, out_cos); }
template<>    inline void sincos_(float x, float& out_sin, float& out_cos) { sincos(x, out_sin, out_cos); }

template<typename T> T     acos_(T x)     { return acos(x); }
template<>    inline float acos_(float x) { return acosf(x); }

template<typename T> bool is_zero(T f) { return fabs_(f) < T(1e-15); }
}

// ax^2 + bx + c = 0
//
//             /---------
//      -b +- V b^2 - 4ac
// x = -------------------
//             2a
template<typename T> int quadratic(T a, T b, T c, T out_roots[2])
{
	const T q = (b * b) - (T(4) * a * c);
	if (q >= T(0) && !detail::is_zero(a))
	{
		const T sq = detail::sqrt_(q);
		const T d = T(1) / (T(2) * a);
		out_roots[0] = (-b + sq) * d;
		out_roots[1] = (-b - sq) * d;
		return 2;
	}

	return 0;
}

template<typename T> int cubic(T a, T b, T c, T d, T out_roots[3])
{
	int	num;
	T sub,
	A, B, C,
	sq_A, p, q,
	cb_p, D;

	// normalize the equation:x ^ 3 + Ax ^ 2 + Bx  + C = 0
	A = c / d;
	B = b / d;
	C = a / d;

	// substitute x = y - A / 3 to eliminate the quadric term: x^3 + px + q = 0

	sq_A = A * A;
	p = T(1)/T(3) * (T(-1)/T(3) * sq_A + B);
	q = T(1)/T(2) * (T(2)/T(27) * A *sq_A - T(1)/T(3) * A * B + C);

	// use Cardano's formula

	cb_p = p * p * p;
	D = q * q + cb_p;

	if (detail::is_zero(D))
	{
		if (detail::is_zero(q))
		{
			// one triple solution
			out_roots[0] = T(0);
			num = 1;
		}
		else
		{
			// one single and one double solution
			T u = detail::cbrt_(-q);
			out_roots[0] = T(2) * u;
			out_roots[1] = - u;
			num = 2;
		}
	}
	else if (D < T(0))
	{
		// casus irreductibilis: three real solutions
		T phi = T(1)/T(3) * detail::acos_(-q / detail::sqrt_(-cb_p));
		T t = T(2) * detail::sqrt_(-p);
		out_roots[0] = t * detail::cos_(phi);

		const T PI = T(3.14159265358979323846);

		out_roots[1] = -t * detail::cos_(phi + PI / T(3));
		out_roots[2] = -t * detail::cos_(phi - PI / T(3));
		num = 3;
	}
	else
	{
		// one real solution
		T sqrt_D = detail::sqrt_(D);
		T u = detail::cbrt_(sqrt_D + detail::fabs_(q));

		if (q > T(0))
			out_roots[0] = - u + p / u ;
		else
			out_roots[0] = u - p / u;
		num = 1;
	}

	// resubstitute
	sub = T(1) / T(3) * A;
	for (int i = 0; i < num; i++)
		out_roots[i] -= sub;
	return num;
}

template<typename T> int quadric(T a, T b, T c, T out_roots[2])
{
	T p, q, D;
	
	// make sure we have a d2 equation
	if (detail::is_zero(c))
	{
		if (detail::is_zero(b))
			return 0;

		out_roots[0] = -a / b;
		return 1;
	}

	// normal for: x^2 + px + q
	p = b / (T(2) * c);
	q = a / c;
	D = p * p - q;

	if (detail::is_zero(D))
	{
		// one double root
		out_roots[0] = out_roots[1] = -p;
		return 1;
	}

	if (D < T(0))
		return 0; // no real root

	else
	{
		// two real roots
		T sqrt_D = detail::sqrt_(D);
		out_roots[0] = sqrt_D - p;
		out_roots[1] = -sqrt_D - p;
		return 2;
	}
}

template<typename T> int quartic(T a, T b, T c, T d, T e, T out_roots[4])
{
	T z, u, v, sub,
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
	p = T(-3) / T(8) * sq_A + B;
	q = T(1) / T(8) * sq_A * A - T(1) / T(2) * A * B + C;
	r = T(-3) / T(256) * sq_A * sq_A + T(1) / T(16) * sq_A * B - T(1) / T(4) * A * C + D;

	if (detail::is_zero(r))
	{
		// no absolute term:y(y ^ 3 + py + q) = 0
		num = cubic(q, p, T(0), T(1), out_roots);
		out_roots[num++] = 0;
	}
	else
	{
		// solve the resolvent cubic...
		cubic(T(0.5) * r * p - T(0.125) * q * q, -r, T(-0.5) * p, T(1), out_roots);

		// ...and take the one real solution...
		z = out_roots[0];

		// ...to build two quadratic equations
		u = z * z - r;
		v = T(2) * z - p;

		if (detail::is_zero(u))
			u = T(0);
		else if (u > T(0))
			u = detail::sqrt_(u);
		else
			return 0;

		if (detail::is_zero(v))
			v = T(0);
		else if (v > T(0))
			v = detail::sqrt_(v);
		else
			return 0;

		num = quadric(z - u, q < 0 ? -v : v, T(1), out_roots);
		num += quadric(z + u, q < 0 ? v : -v, T(1), out_roots + num);
	}

	// resubstitute
	sub = T(1) / T(4) * A;
	for (int i = 0; i < num; i++)
		out_roots[i] -= sub;

	return num;
}

}
