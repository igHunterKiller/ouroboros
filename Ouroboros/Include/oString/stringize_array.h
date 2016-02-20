// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// implement stringize for arrays of values

#pragma once
#include <oCore/stringize.h>

namespace ouro {

// _____________________________________________________________________________
// Values separated by whitespace

bool from_string_float_array(float* out_values, size_t num_values, const char* src);
bool from_string_double_array(double* out_values, size_t num_values, const char* src);

}
