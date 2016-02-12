// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

namespace ouro {

// Forward biorthogonal CDF 9/7 wavelet transform. Transforms in place.
void cdf97fwd(float* values, unsigned int num_values);

// Inverse biorthogonal CDF 9/7 wavelet transform. Transforms in place.
void cdf97inv(float* values, unsigned int num_values);

}
