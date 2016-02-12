// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Common floating-point/math types for C++ or HLSL

#ifndef oHLSL
	#pragma once
#endif
#ifndef oMath_floats_h
#define oMath_floats_h

#ifdef oHLSL
	#define FLT_EPSILON               1.192092896e-07F /* smallest such that 1.0+FLT_EPSILON != 1.0 */
	#define FLT_MAX                   3.402823466e+38F /* max value */
#else
	#include <float.h>
#endif

#define oPI                         (3.14159265358979323846)
#define oINVPI                      (0.31830988618379067154)
#define oE                          (2.71828183)
#define oSQRT2                      (1.41421356237309504880)
#define oHALF_SQRT2                 (0.70710678118654752440)
#define oTHREE_QUARTERS_SQRT3       (0.86602540378443860)
#define oVERY_SMALL                 (0.000001)

#define oPIf                        (float(oPI))
#define oINVPIf                     (float(oINVPI))
#define oEf                         (float(oE))
#define oSQRT2f                     (float(oSQRT2))
#define oHALF_SQRT2f                (float(oHALF_SQRT2))
#define oTHREE_QUARTERS_SQRT3f      (float(oTHREE_QUARTERS_SQRT3))
#define oVERY_SMALLf                (float(oVERY_SMALL))

// Common frustum-related values
#define oDEFAULT_NEAR_CLIPf         (0.1f)
#define oDEFAULT_FAR_CLIPf          (1000.0f)
#define oDEFAULT_FOVX_RADIANSf      (oPIf / 4.0f)
#define oDEFAULT_FOVY_RADIANSf      (oPIf / 4.0f)

#define oDEFAULT_ASPECT_RATIOf      (16.0f / 9.0f)

// Because TVs are further away and PCs are closer, but people's field 
// of view doesn't change, it's how much of the screen they can see.
#define oRECOMMENDED_TV_FOVX_RADIANSf (oPIf / 3.0f)
#define oRECOMMENDED_PC_FOVX_RADIANSf (oPIf / 4.0f)

#endif
