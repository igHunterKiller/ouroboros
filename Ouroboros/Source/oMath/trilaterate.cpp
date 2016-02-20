// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/trilaterate.h>
#include <oMath/matrix.h>

namespace ouro {

float trilaterate(const float3 in_observers[4], const float in_distances[4], float3* out_position)
{
	float3 observers[4];
	float distances[4];
	for (int i = 0; i < 4; i++)
	{
		observers[i].x = in_observers[i].x;
		observers[i].y = in_observers[i].y;
		observers[i].z = in_observers[i].z;
		distances[i] = in_distances[i];
	}

	float minError = FLT_MAX;
	float3 bestEstimate;

	// We try four times because one point is a tie-breaker and we want the best approximation
	for (int i = 0; i < 4; i++)
	{
		// Switch which observer is the tie-breaker (index 3)
		float3 oldZero = observers[0];
		float oldZeroDistance = distances[0];
		for (int j = 0; j < 3; j++)
		{
			observers[j] = observers[j+1];
			distances[j] = distances[j+1];
		}
		observers[3] = oldZero;
		distances[3] = oldZeroDistance;

		// Translate and rotate all observers such that the 
		// first three observers lie on the z=0 plane this
		// simplifies the system of equations necessary to perform
		// trilateration to the following: 
		// r1sqrd = xsqrd			+ ysqrd			+ zsqrd
		// r2sqrd = (x - x1)^2	+ ysqrd			+ zsqrd
		// r3sqrd = (x - x2)^2	+ (y - y2) ^2 + zsqrd
		float4x4 inverseTransform;
		float3 transformedObservers[4];
		{
			// Translate everything such that the first point is at the orgin (collapsing the x y and z components)
			float3 translation = -observers[0];
			float4x4 completeTransform = translate(translation);

			for (int j = 0; j < 4; j++)
				transformedObservers[j] = observers[j] + translation;

			// Rotate everything such that the second point lies on the X-axis (collapsing the y and z components)
			float4x4 rot =  rotate(normalize(transformedObservers[1]), float3(1.0f, 0.0f, 0.0f)); 
			for (int j = 1; j < 4; j++)
				transformedObservers[j] = mul(rot, float4(transformedObservers[j], 1.0f)).xyz();

			// Add the rotation to our transform
			completeTransform = mul(rot, completeTransform);

			// Rotate everything such that the third point lies on the Z-plane (collapsing the z component)
			const float3& poi = transformedObservers[2];
			float rad = acos(normalize(float3(0.0f, poi.y, poi.z)).y);
			rad *= poi.z < 0.0f ? 1.0f : -1.0f;

			rot = rotate(rad, float3(1.0f, 0.0f, 0.0f));
			for (int j = 1; j < 4; j++)
				transformedObservers[j] = mul(rot, float4(transformedObservers[j], 1.0f)).xyz();

			// Add the rotation to our transform
			completeTransform = mul(rot, completeTransform);

			// Invert so that we can later move back to the original space
			inverseTransform = invert(completeTransform);
		}

		// Trilaterate the postion in the transformed space
		float3 triPos;
		{
			const float x1 = transformedObservers[1][0];
			const float x1sqrd = x1 * x1;
			const float x2 = transformedObservers[2][0];
			const float x2sqrd = x2 * x2;
			const float y2 = transformedObservers[2][1];
			const float y2sqrd = y2 * y2;

			const float r1sqrd = distances[0] * distances[0];
			const float r2sqrd = distances[1] * distances[1];
			const float r3sqrd = distances[2] * distances[2];

			// Solve for x
			float x = (r1sqrd - r2sqrd + x1sqrd) / (2 * x1);

			// Solve for y
			float y = ((r1sqrd - r3sqrd + x2sqrd + y2sqrd) / (2 * y2)) - ((x2 / y2) * x);

			// Solve positive Z
			float zsqrd = (r1sqrd - (x * x) - (y * y));
			if (zsqrd < 0.0)
				continue;

			// Use the fourth point as a tie-breaker
			float posZ = sqrt(zsqrd);
			triPos = float3(x, y, posZ);
			float posDistToFourth = abs(distance(triPos, transformedObservers[3]) - distances[3]);
			float negDistToFourth = abs(distance(float3(triPos.xy(), -triPos[2]), transformedObservers[3]) - distances[3]);
			if (negDistToFourth < posDistToFourth)
				triPos.z = -triPos.z;

			float error = __min(posDistToFourth, negDistToFourth);
			if (error < minError)
			{
				minError = error;
				bestEstimate = mul(inverseTransform, float4(triPos, 1.0f)).xyz();
			}
		}

	}

	// Return the trilaterated position in the original space
	*out_position = bestEstimate;
	return minError;
}

// Helper function for coordinate_transform that determines where a position 
// lies in another coordinate system.
static bool position_in_start_coordinates(const float3 from_coords[4], const float3 to_coords[4], const float3& _EndPos, float3* out_position)
{
	float distances[4];
	for (int i = 0; i < 4; i++)
		distances[i] = distance(to_coords[i], _EndPos);
	return trilaterate(from_coords, distances, out_position) < float(10);
}

bool coordinate_transform(const float3 from_coords[4], const float3 to_coords[4], float4x4* out_matrix)
{
	float3 orgin;
	float3 posXAxis;
	float3 posYAxis;
	float3 posZAxis;

	// Trilaterate the orgin an axis
	if (!position_in_start_coordinates(from_coords, to_coords, float3(0.0f, 0.0f, 0.0f), &orgin)) return false;
	if (!position_in_start_coordinates(from_coords, to_coords, float3(1.0f, 0.0f, 0.0f), &posXAxis)) return false;
	if (!position_in_start_coordinates(from_coords, to_coords, float3(0.0f, 1.0f, 0.0f), &posYAxis)) return false;
	if (!position_in_start_coordinates(from_coords, to_coords, float3(0.0f, 0.0f, 1.0f), &posZAxis)) return false;

	// Normalize axis
	posXAxis = normalize(posXAxis - orgin);
	posYAxis = normalize(posYAxis - orgin);
	posZAxis = normalize(posZAxis - orgin);

	float4x4 transform;
	transform[0] = float4(posXAxis, 0.0f);
	transform[1] = float4(posYAxis, 0.0f);
	transform[2] = float4(posZAxis, 0.0f);
	transform[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);

	transform = invert(transform);
	transform = mul(transform, translate(-orgin));

	*out_matrix = transform;
	return true;
}

}
