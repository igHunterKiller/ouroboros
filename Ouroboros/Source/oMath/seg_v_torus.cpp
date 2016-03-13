// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/seg_v_torus.h>
#include <oMath/seg_v_sphere.h>
#include <oMath/matrix.h>
#include <oMath/polynomial.h>
#include <algorithm>

// Cychosz, Joseph M., Intersecting a Ray with An Elliptical Torus, Graphics Gems II, p. 251-256
// http://tog.acm.org/resources/GraphicsGems/gemsii/intersect/
static int inttor(const float3& raybase, const float3& raycos, const float3& center, float radius, float rplane, float rnorm, const float4x4& tran, int* nhits, float rhits[4])
{
	int	hit;			/* True if ray intersects torus	*/
	float	rsphere;		/* Bounding sphere radius	*/
	float3	Base, Dcos;		/* Transformed intersection ray	*/
	float	rmin, rmax;		/* Root bounds			*/
	float	yin, yout;
	float	rho, a0, b0;		/* Related constants		*/
	float	f, l, t, g, q, m, u;	/* Ray dependent terms		*/
	float	C[5];			/* Quartic coefficients		*/

	*nhits  = 0;

/*	Compute the intersection of the ray with a bounding sphere.	*/

	rsphere = radius + max (rplane,rnorm);

	hit = ouro::ray_v_sphere(raybase, raycos, float4(center, rsphere), &rmin, &rmax);

	if  (!hit) return (hit);	/* If ray misses bounding sphere*/

/*	Transform the intersection ray					*/

	Base = raybase;
	Dcos = raycos;

	Base = mul(tran, float4(Base, 1.0f)).xyz(); //V3MulPointByMatrix  (&Base,&tran);
	Dcos = mul((float3x3)tran, Dcos); //V3MulVectorByMatrix (&Dcos,&tran);

/*	Bound the torus by two parallel planes rnorm from the x-z plane.*/

	yin  = Base.y + rmin * Dcos.y;
	yout = Base.y + rmax * Dcos.y;
	hit  = !( (yin >  rnorm && yout >  rnorm) ||
		  (yin < -rnorm && yout < -rnorm) );

	if  (!hit) return (hit);	/* If ray is above/below torus.	*/

/*	Compute constants related to the torus.				*/

	rho = rplane*rplane / (rnorm*rnorm);
	a0  = 4.0f * radius*radius;
	b0  = radius*radius - rplane*rplane;

/*	Compute ray dependent terms.					*/

	f = 1.0f - Dcos.y*Dcos.y;
	l = 2.0f * (Base.x*Dcos.x + Base.z*Dcos.z);
	t = Base.x*Base.x + Base.z*Base.z;
	g = f + rho * Dcos.y*Dcos.y;
	q = a0 / (g*g);
	m = (l + 2.0f*rho*Dcos.y*Base.y) / g;
	u = (t +    rho*Base.y*Base.y + b0) / g;

/*	Compute the coefficients of the quartic.			*/

	C[0] = 1.0f;
	C[1] = 2.0f * m;
	C[2] = m*m + 2.0f*u - q*f;
	C[3] = 2.0f*m*u - q*l;
	C[4] = u*u - q*t;
	
/*	Use quartic root solver found in "Graphics Gems" by Jochen	*/
/*	Schwarze.							*/

	*nhits = ouro::quartic(C[0], C[1], C[2], C[3], C[4],rhits);

/*	SolveQuartic returns root pairs in reversed order.		*/
	//m = rhits[0]; u = rhits[1]; rhits[0] = u; rhits[1] = m;
	//m = rhits[2]; u = rhits[3]; rhits[2] = u; rhits[3] = m;

	return (*nhits != 0);
}

namespace ouro {

int seg_v_torus(const float3& a0, const float3& a1, const float3& center, const float3& normalized_axis, float major_radius, float minor_radius, float* at0, float* at1, float* at2, float* at3)
{
	// rotate segment into local space (Y up for the original algorithm)
	float4x4 tx = translate(-center) * rotate(normalized_axis, float3(0.0f, 1.0f, 0.0f));

	float roots[4];
	int n;
	if (inttor(a0, normalize(a1-a0), center, major_radius, minor_radius, minor_radius, tx, &n, roots))
	{
		std::sort(roots, roots + n);

		if (n > 0) *at0 = roots[0];
		if (n > 1) *at1 = roots[1];
		if (n > 2) *at2 = roots[2];
		if (n > 3) *at3 = roots[3];
	}
	return n;
}

}
