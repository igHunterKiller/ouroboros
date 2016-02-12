// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/equal.h>

using namespace ouro;

oTEST(oMath_equal)
{
	oCHECK(!equal(1.0f, 1.000001f), "equal() failed");
	oCHECK(equal(1.0f, 1.000001f, 8), "equal() failed");
	oCHECK(!equal(2.0, 1.99999999999997), "equal failed");
	oCHECK(equal(2.0, 1.99999999999997, 135), "equal failed");
}
