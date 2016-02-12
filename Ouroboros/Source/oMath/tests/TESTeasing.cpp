// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMath/equal.h>
#include <oMath/easing.h>

using namespace ouro;

oTEST(oMath_easing)
{
	float result;
	// linear
	result = simple_linear_tween(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.6f), "simple_linear_tween returned incorrect value");
	result = simple_linear_tween(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.6857142f), "simple_linear_tween returned incorrect value");
	result = simple_linear_tween(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "simple_linear_tween returned incorrect value");

	// quadratic
	result = quadratic_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.28f), "quadratic_ease_in returned incorrect value");
	result = quadratic_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.117551f), "quadratic_ease_in returned incorrect value");
	result = quadratic_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quadratic_ease_in returned incorrect value");

	result = quadratic_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.92f), "quadratic_ease_out returned incorrect value");
	result = quadratic_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 2.2538776f), "quadratic_ease_out returned incorrect value");
	result = quadratic_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quadratic_ease_out returned incorrect value");

	result = quadratic_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.84f), "quadratic_ease_inout returned incorrect value");
	result = quadratic_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.235102f), "quadratic_ease_inout returned incorrect value");
	result = quadratic_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quadratic_ease_inout returned incorrect value");

	// cubic
	result = cubic_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.024f), "cubic_ease_in returned incorrect value");
	result = cubic_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0201516f), "cubic_ease_in returned incorrect value");
	result = cubic_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "cubic_ease_in returned incorrect value");

	result = cubic_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.984f), "cubic_ease_out returned incorrect value");
	result = cubic_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 2.7246414f), "cubic_ease_out returned incorrect value");
	result = cubic_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "cubic_ease_out returned incorrect value");

	result = cubic_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.936f), "cubic_ease_inout returned incorrect value");
	result = cubic_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0806065f), "cubic_ease_inout returned incorrect value");
	result = cubic_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "cubic_ease_inout returned incorrect value");

	// quartic
	result = quartic_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 0.8192f), "quartic_ease_in returned incorrect value");
	result = quartic_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0034546f), "quartic_ease_in returned incorrect value");
	result = quartic_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quartic_ease_in returned incorrect value");

	result = quartic_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.9967999f), "quartic_ease_out returned incorrect value");
	result = quartic_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 3.1147029f), "quartic_ease_out returned incorrect value");
	result = quartic_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quartic_ease_out returned incorrect value");

	result = quartic_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(result == 1.9744f, "quartic_ease_inout returned incorrect value");
	result = quartic_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(result == 1.0276365f, "quartic_ease_inout returned incorrect value");
	result = quartic_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(result == 5.f, "quartic_ease_inout returned incorrect value");

	// quintic
	result = quintic_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 0.65535998f), "quintic_ease_in returned incorrect value");
	result = quintic_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0005922f), "quintic_ease_in returned incorrect value");
	result = quintic_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quintic_ease_in returned incorrect value");

	result = quintic_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.99936f), "quintic_ease_out returned incorrect value");
	result = quintic_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 3.4378967f), "quintic_ease_out returned incorrect value");
	result = quintic_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quintic_ease_out returned incorrect value");

	result = quintic_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.98976f), "quintic_ease_inout returned incorrect value");
	result = quintic_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0094754f), "quintic_ease_inout returned incorrect value");
	result = quintic_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "quintic_ease_inout returned incorrect value");

	// sin
	result = sin_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.3819660f), "sin_ease_in returned incorrect value");
	result = sin_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.1441486f), "sin_ease_in returned incorrect value");
	result = sin_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "sin_ease_in returned incorrect value");

	result = sin_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.9021131f), "sin_ease_out returned incorrect value");
	result = sin_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 2.0641475f), "sin_ease_out returned incorrect value");
	result = sin_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "sin_ease_out returned incorrect value");

	result = sin_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.8090169f), "sin_ease_inout returned incorrect value");
	result = sin_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.2831024f), "sin_ease_inout returned incorrect value");
	result = sin_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "sin_ease_inout returned incorrect value");

	// exponential
	result = exponential_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 0.5f), "exponential_ease_in returned incorrect value");
	result = exponential_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0128177f), "exponential_ease_in returned incorrect value");
	result = exponential_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "exponential_ease_in returned incorrect value");

	result = exponential_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.9921875f), "exponential_ease_out returned incorrect value");
	result = exponential_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 3.7809863f), "exponential_ease_out returned incorrect value");
	result = exponential_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(result > 4.99 && result < 5.01, "exponential_ease_out returned incorrect value");

	result = exponential_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.984375f), "exponential_ease_inout returned incorrect value");
	result = exponential_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0210297f), "exponential_ease_inout returned incorrect value");
	result = exponential_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(result > 4.99 && result < 5.01, "exponential_ease_inout returned incorrect value");

	// circular
	result = circular_ease_in(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 0.8f), "circular_ease_in returned incorrect value");
	result = circular_ease_in(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.0592138f), "circular_ease_in returned incorrect value");
	result = circular_ease_in(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "circular_ease_in returned incorrect value");

	result = circular_ease_out(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.9595917f), "circular_ease_out returned incorrect value");
	result = circular_ease_out(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 3.2395335f), "circular_ease_out returned incorrect value");
	result = circular_ease_out(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "circular_ease_out returned incorrect value");

	result = circular_ease_inout(0.8f, 0.f, 2.f, 1.0f);
	oCHECK(equal(result, 1.9165151f), "circular_ease_inout returned incorrect value");
	result = circular_ease_inout(0.3f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 1.1212249f), "circular_ease_inout returned incorrect value");
	result = circular_ease_inout(1.75f, 1.f, 4.f, 1.75f);
	oCHECK(equal(result, 5.f), "circular_ease_inout returned incorrect value");
}
