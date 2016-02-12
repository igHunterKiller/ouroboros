// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// an implementation of simplex noise along with fractal brownian 
// motion and turbulence.

http://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&ved=0CCIQFjAA&url=http%3A%2F%2Fwww.itn.liu.se%2F~stegu%2Fsimplexnoise%2Fsimplexnoise.pdf&ei=nzZRUMe-JKOtigLDpYBQ&usg=AFQjCNEVzOM03haFrTgLrjJp-jPkQyTOKA

#ifndef oHLSL
	#pragma once
#endif
#ifndef oMath_simplex_h
#define oMath_simplex_h

#include <oMath/hlsl.h>

static const float3 simplex_grad3[12] =
{	float3(1,1,0), float3(-1,1,0), float3(1,-1,0), float3(-1,-1,0),
	float3(1,0,1), float3(-1,0,1), float3(1,0,-1), float3(-1,0,-1),
	float3(0,1,1), float3(0,-1,1), float3(0,1,-1), float3(0,-1,-1)
};

static const int simplex_perm_table[512] = {151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

inline int simplex_perm(int i)
{
	return simplex_perm_table[i & 255];
}

// returns simplex noise (Ken Perlin's extended noise) for 3 dimensions on [-1,1]
inline float simplex_noise(oIN(float3, v))
{
	float n0, n1, n2, n3; // Noise contributions from the four corners
	// Skew the input space to determine which simplex cell we're in
	const float F3 = 1.0f/3.0f;
	float s = (v.x+v.y+v.z)*F3; // Very nice and simple skew factor for 3D
	int3 ijk = floor(v+s);
	const float G3 = 1.0f/6.0f; // Very nice and simple unskew factor, too
	float t = (ijk.x+ijk.y+ijk.z)*G3;
	float3 V0 = float3(float3(ijk)-t); // Unskew the cell origin back to (x,y,z) space
	float3 v0 = float3(v-V0); // The x,y,z distances from the cell origin
	// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
	// Determine which simplex we are in.
	uint3 ijk1; // Offsets for second corner of simplex in (i,j,k) coords
	uint3 ijk2; // Offsets for third corner of simplex in (i,j,k) coords
	if (v0.x >= v0.y)
	{
		if (v0.y >= v0.z) { ijk1 = uint3(1,0,0); ijk2 = uint3(1,1,0); } // X Y Z order
		else if (v0.x >= v0.z) { ijk1 = uint3(1,0,0); ijk2 = uint3(1,0,1); } // X Z Y order
		else { ijk1 = uint3(0,0,1); ijk2 = uint3(1,0,1); } // Z X Y order
	}

	else // v0.x < v0.y
	{ 
		if (v0.y < v0.z) { ijk1 = uint3(0,0,1); ijk2 = uint3(0,1,1); } // Z Y X order
		else if (v0.x < v0.z) { ijk1 = uint3(0,1,0); ijk2 = uint3(0,1,1); } // Y Z X order
		else { ijk1 = uint3(0,1,0); ijk2 = uint3(1,1,0); } // Y X Z order
	}

	// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
	// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
	// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
	// c = 1/6.
	float3 v1 = v0 - float3(ijk1) + G3; // Offsets for second corner in (x,y,z) coords
	float3 v2 = v0 - float3(ijk2) + 2.0f*G3; // Offsets for third corner in (x,y,z) coords
	float3 v3 = v0 - 1.0f + 3.0f*G3; // Offsets for last corner in (x,y,z) coords

	// Work out the hashed gradient indices of the four simplex corners
	uint3 ijk255 = ijk & 255;

	const uint TWELVE = 12; // for uint to squelch as warning about signed modulus
	uint gi0 = simplex_perm(ijk255.x+simplex_perm(ijk255.y+simplex_perm(ijk255.z))) % TWELVE;
	uint gi1 = simplex_perm(ijk255.x+ijk1.x+simplex_perm(ijk255.y+ijk1.y+simplex_perm(ijk255.z+ijk1.z))) % TWELVE;
	uint gi2 = simplex_perm(ijk255.x+ijk2.x+simplex_perm(ijk255.y+ijk2.y+simplex_perm(ijk255.z+ijk2.z))) % TWELVE;
	uint gi3 = simplex_perm(ijk255.x+1+simplex_perm(ijk255.y+1+simplex_perm(ijk255.z+1))) % TWELVE;

	// Calculate the contribution from the four corners

	float t0 = 0.6f - dot(v0,v0);
	if(t0<0) n0 = 0.0f;
	else {
		t0 *= t0;
		n0 = t0 * t0 * dot(simplex_grad3[gi0], v0);
	}
	float t1 = 0.6f - dot(v1,v1);
	if(t1<0) n1 = 0.0f;
	else {
		t1 *= t1;
		n1 = t1 * t1 * dot(simplex_grad3[gi1], v1);
	}
	float t2 = 0.6f - dot(v2,v2);
	if(t2<0) n2 = 0.0f;
	else {
		t2 *= t2;
		n2 = t2 * t2 * dot(simplex_grad3[gi2], v2);
	}
	float t3 = 0.6f - dot(v3,v3);
	if(t3<0) n3 = 0.0f;
	else {
		t3 *= t3;
		n3 = t3 * t3 * dot(simplex_grad3[gi3], v3);
	}
	// Add contributions from each corner to get the final noise value.
	// The result is scaled to stay just inside [-1,1]
	return 32.0f*(n0 + n1 + n2 + n3);
}

// Fractal Brownian motion (aka fBM, layered Perlin noise)
inline float simplex_fbm(oIN(float3, coord), uint num_octaves, float lacunarity, float gain)
{
	// http://www.scribd.com/doc/39470687/27/fBm-Shader-Code

	float amp = 1;
	float amp_sum = 1;
	float3 C = coord;
	float n = simplex_noise(coord);
	for (uint i = 1; i < num_octaves; i += 1)
	{
		amp *= gain;
		amp_sum += amp;
		C *= lacunarity;
		n += amp * simplex_noise(C);
	}
	return n / amp_sum;
}

// fBM, using abs() of noise() function.
inline float simplex_turbulence(oIN(float3, coord), uint num_octaves, float lacunarity, float gain)
{
	// http://www.scribd.com/doc/39470687/27/fBm-Shader-Code

	float sum = 0;
	float amp = 1;
	float amp_sum = 0;
	float3 c = coord;
	for (uint i = 0; i < num_octaves; i++)
	{
		sum += amp * (0.5f+0.5f*simplex_noise(c));
		amp_sum += amp;
		amp *= gain;
		c *= lacunarity;
	}
	return sum / amp_sum;
}

#endif
