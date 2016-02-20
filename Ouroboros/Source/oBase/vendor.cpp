// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/stringize.h>
#include <oBase/vendor.h>

namespace ouro {

template<> const char* as_string<vendor>(const vendor& v)
{
	const char* s_names[] =
	{
		"unknown",
		"amd",
		"Apple",
		"ARM",
		"Intel",
		"internal",
		"LG",
		"Maxtor",
		"Microsoft",
		"Nintendo",
		"NVIDIA",
		"SanDisk",
		"Samsung",
		"Sony",
		"Vizio",
		"Western Digital",
	};

	return detail::counted_enum_as_string(v, s_names);
}

}
