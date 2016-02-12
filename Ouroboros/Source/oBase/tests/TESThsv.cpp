// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>
#include <oMath/hlslx.h>

using namespace ouro;

// http://en.wikipedia.org/wiki/HSV_color_space
// The original source lists H in degrees, but it's more useful to store it as a 
// unorm, so normalize it here
//#define DEGREES(x) x
#define DEGREES(x) (x / 360.0f)

struct HSV_TEST
{
	const char* Tag;
	float R, G, B, H, H2, C, C2, V, L, I, Y601, Shsv, Shsl, Shsi;
};

static const HSV_TEST kHSVTests[] = 
{
	{ "#FFFFFF", 1.000f, 1.000f, 1.000f, DEGREES(0.0f), DEGREES(0.0f), 0.000f, 0.000f, 1.000f, 1.000f, 1.000f, 1.000f, 0.000f, 0.000f, 0.000f, },
	{ "#808080", 0.500f, 0.500f, 0.500f, DEGREES(0.0f), DEGREES(0.0f), 0.000f, 0.000f, 0.500f, 0.500f, 0.500f, 0.500f, 0.000f, 0.000f, 0.000f, },
	{ "#000000", 0.000f, 0.000f, 0.000f, DEGREES(0.0f), DEGREES(0.0f), 0.000f, 0.000f, 0.000f, 0.000f, 0.000f, 0.000f, 0.000f, 0.000f, 0.000f, },
	{ "#FF0000", 1.000f, 0.000f, 0.000f, DEGREES(0.0f), DEGREES(0.0f), 1.000f, 1.000f, 1.000f, 0.500f, 0.333f, 0.299f, 1.000f, 1.000f, 1.000f, },
	{ "#BFBF00", 0.750f, 0.750f, 0.000f, DEGREES(60.0f), DEGREES(60.0f), 0.750f, 0.750f, 0.750f, 0.375f, 0.500f, 0.664f, 1.000f, 1.000f, 1.000f, },
	{ "#008000", 0.000f, 0.500f, 0.000f, DEGREES(120.0f), DEGREES(120.0f), 0.500f, 0.500f, 0.500f, 0.250f, 0.167f, 0.293f, 1.000f, 1.000f, 1.000f, },
	{ "#80FFFF", 0.500f, 1.000f, 1.000f, DEGREES(180.0f), DEGREES(180.0f), 0.500f, 0.500f, 1.000f, 0.750f, 0.833f, 0.850f, 0.500f, 1.000f, 0.400f, },
	{ "#8080FF", 0.500f, 0.500f, 1.000f, DEGREES(240.0f), DEGREES(240.0f), 0.500f, 0.500f, 1.000f, 0.750f, 0.667f, 0.557f, 0.500f, 1.000f, 0.250f, },
	{ "#BF40BF", 0.750f, 0.250f, 0.750f, DEGREES(300.0f), DEGREES(300.0f), 0.500f, 0.500f, 0.750f, 0.500f, 0.583f, 0.457f, 0.667f, 0.500f, 0.571f, },
	{ "#A0A424", 0.628f, 0.643f, 0.142f, DEGREES(61.8f), DEGREES(61.5f), 0.501f, 0.494f, 0.643f, 0.393f, 0.471f, 0.581f, 0.779f, 0.638f, 0.699f, },
	{ "#411BEA", 0.255f, 0.104f, 0.918f, DEGREES(251.1f), DEGREES(250.0f), 0.814f, 0.750f, 0.918f, 0.511f, 0.426f, 0.242f, 0.887f, 0.832f, 0.756f, },
	{ "#1EAC41", 0.116f, 0.675f, 0.255f, DEGREES(134.9f), DEGREES(133.8f), 0.559f, 0.504f, 0.675f, 0.396f, 0.349f, 0.460f, 0.828f, 0.707f, 0.667f, },
	{ "#F0C80E", 0.941f, 0.785f, 0.053f, DEGREES(49.5f), DEGREES(50.5f), 0.888f, 0.821f, 0.941f, 0.497f, 0.593f, 0.748f, 0.944f, 0.893f, 0.911f, },
	{ "#B430E5", 0.704f, 0.187f, 0.897f, DEGREES(283.7f), DEGREES(284.8f), 0.710f, 0.636f, 0.897f, 0.542f, 0.596f, 0.423f, 0.792f, 0.775f, 0.686f, },
	{ "#ED7651", 0.931f, 0.463f, 0.316f, DEGREES(14.3f), DEGREES(13.2f), 0.615f, 0.556f, 0.931f, 0.624f, 0.570f, 0.586f, 0.661f, 0.817f, 0.446f, },
	{ "#FEF888", 0.998f, 0.974f, 0.532f, DEGREES(56.9f), DEGREES(57.4f), 0.466f, 0.454f, 0.998f, 0.765f, 0.835f, 0.931f, 0.467f, 0.991f, 0.363f, },
	{ "#19CB97", 0.099f, 0.795f, 0.591f, DEGREES(162.4f), DEGREES(163.4f), 0.696f, 0.620f, 0.795f, 0.447f, 0.495f, 0.564f, 0.875f, 0.779f, 0.800f, },
	{ "#362698", 0.211f, 0.149f, 0.597f, DEGREES(248.3f), DEGREES(247.3f), 0.448f, 0.420f, 0.597f, 0.373f, 0.319f, 0.219f, 0.750f, 0.601f, 0.533f, },
	{ "#7E7EB8", 0.495f, 0.493f, 0.721f, DEGREES(240.5f), DEGREES(240.4f), 0.228f, 0.227f, 0.721f, 0.607f, 0.570f, 0.520f, 0.316f, 0.290f, 0.135f, },
};

oTEST(oBase_hsv)
{
	static const float HSV_EPS = 0.001f;

	int i = 0;
	for (const auto& c : kHSVTests)
	{
		const float3 kRGB(c.R, c.G, c.B);
		const float3 kHSV(c.H, c.Shsv, c.V);

		float3 testRGB = hsvtorgb(kHSV);
		oCHECK(equal_eps(testRGB, kRGB, HSV_EPS), "failed (%d \"%s\"): oHSVtoRGB(%.03f, %.03f, %.03f) expected (%.03f, %.03f, %.03f), got (%.03f, %.03f, %.03f)"
			, i, c.Tag
			, kHSV.x, kHSV.y, kHSV.z
			, kRGB.x, kRGB.y, kRGB.z
			, testRGB.x, testRGB.y, testRGB.z);

		float3 testHSV = rgbtohsv(kRGB);
		oCHECK(equal_eps(testHSV, kHSV, HSV_EPS), "failed (%d \"%s\"): oRGBtoHSV(%.03f, %.03f, %.03f) expected (%.03f, %.03f, %.03f), got (%.03f, %.03f, %.03f)"
				, i, c.Tag
				, kRGB.x, kRGB.y, kRGB.z
				, kHSV.x, kHSV.y, kHSV.z
				, testHSV.x, testHSV.y, testHSV.z);
		i++;
	}
}
