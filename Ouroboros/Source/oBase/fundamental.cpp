// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/fundamental.h>
#include <oCore/stringize.h>

namespace ouro {

template<> const char* as_string(const fundamental& f)
{
	static const char* s_names[] =
	{
		"unknown type",
		"void",
		"bool",
		"char",
		"unsigned char",
		"wchar",
		"short",
		"unsigned short",
		"int",
		"unsigned int",
		"long",
		"unsigned long",
		"long long",
		"unsigned long long",
		"float",
		"double",
	};
	return as_string(f, s_names);
}

oDEFINE_TO_FROM_STRING(fundamental);

char to_code(const fundamental& f)
{
	// todo: assign a symbol for each

	static const char s_codes[] =
	{
		'?',
		'v',
		'B',
		'c',
		'b',
		'w',
		's',
		'S',
		'i',
		'u',
		'i',
		'u',
		'I',
		'U',
		'f',
		'd',
	};
	match_array_e(s_codes, fundamental);
	return s_codes[(int)f];
}

fundamental from_code(char c)
{
	static fundamental s_ascii[] =
	{
		fundamental::bool_type,    // ASCII 66
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::llong_type,   // ASCII 73
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::ushort_type,  // ASCII 83
		fundamental::unknown,
		fundamental::ullong_type,  // ASCII 85
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::uchar_type,   // ASCII 98
		fundamental::char_type,    // ASCII 99
		fundamental::double_type,  // ASCII 100
		fundamental::unknown,
		fundamental::float_type,   // ASCII 102
		fundamental::unknown,
		fundamental::unknown,
		fundamental::int_type,     // ASCII 105
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::unknown,
		fundamental::short_type,   // ASCII 115
		fundamental::unknown,
		fundamental::uint_type,    // ASCII 117
		fundamental::void_type,    // ASCII 118
	};

	int i = c - 'B';
	return (i >= 0 && i <= countof(s_ascii)) ? s_ascii[i] : fundamental::unknown;
}

}
