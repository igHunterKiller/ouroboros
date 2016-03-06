// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/vector_component.h>
#include <oCore/stringize.h>

namespace ouro {

template<> const char* as_string(const vector_component& c)
{
	static const char* s_names[] = { "x", "y", "z", "w" };
	match_array_e(s_names, vector_component);
	return (int)c < 0 ? "none" : s_names[(int)c];
}

oDEFINE_TO_FROM_STRING(vector_component);

}
