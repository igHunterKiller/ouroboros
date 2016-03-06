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

size_t fundamental_size(const fundamental& f)
{
	// todo: assign a symbol for each

	static const unsigned char s_sizes[] =
	{
		0,
		0,
		1,
		1,
		1,
		2,
		2,
		2,
		4,
		4,
		4,
		4,
		8,
		8,
		4,
		8,
	};
	match_array_e(s_sizes, fundamental);
	return s_sizes[(int)f];
}

const char* to_format(const fundamental& f)
{
	static const char* s_formats[] =
	{
		"?",
		"void",
		"%d",
		"%c",
		"%u",
		"%lc",
		"%d",
		"%u",
		"%d",
		"%u",
		"%d",
		"%u",
		"%lld",
		"%llu",
		"%f",
		"%f",
	};
	return as_string(f, s_formats);
}

char fundamental_to_code(const fundamental& f)
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

fundamental fundamental_from_code(char c)
{
	static fundamental s_ascii[] =
	{
		fundamental::bool_type,    // ASCII 66
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::llong_type,   // ASCII 73
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::ushort_type,  // ASCII 83
		fundamental::unknown_type,
		fundamental::ullong_type,  // ASCII 85
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::uchar_type,   // ASCII 98
		fundamental::char_type,    // ASCII 99
		fundamental::double_type,  // ASCII 100
		fundamental::unknown_type,
		fundamental::float_type,   // ASCII 102
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::int_type,     // ASCII 105
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::unknown_type,
		fundamental::short_type,   // ASCII 115
		fundamental::unknown_type,
		fundamental::uint_type,    // ASCII 117
		fundamental::void_type,    // ASCII 118
	};

	int i = c - 'B';
	return (i >= 0 && i <= countof(s_ascii)) ? s_ascii[i] : fundamental::unknown_type;
}

static size_t to_string__(char* dst, size_t dst_size, const fundamental& type, const void* src)
{
	#define PR(fmt, type) return snprintf(dst, dst_size, fmt, *(const type*)src);
	switch (type)
	{
		// reordered by expected frequency of access
		case fundamental::float_type:  PR("%f",   float);
		case fundamental::bool_type:   return snprintf(dst, dst_size, "%s", *(const bool*)src ? "true" : "false");
		case fundamental::int_type:
		case fundamental::long_type:   PR("%d",   int);
		case fundamental::uint_type:
		case fundamental::ulong_type:  PR("%u",   unsigned int);
		case fundamental::short_type:  PR("%d",   short);
		case fundamental::ushort_type: PR("%u",   unsigned short);
		case fundamental::llong_type:  PR("%lld", long long);
		case fundamental::ullong_type: PR("%llu", unsigned long long);
		case fundamental::char_type:   PR("%c",   char);
		case fundamental::uchar_type:  PR("%u",   unsigned char);
		case fundamental::wchar_type:  PR("%lc",  wchar_t);
		case fundamental::double_type: PR("%f",   double);
		default: break;
	}
	return size_t(-1);
}

size_t fundamental_to_string(char* dst, size_t dst_size, const fundamental& type, const void* src, size_t num_elements)
{
	size_t offset = 0;
	auto p        = (const char*)src;
	auto sz       = fundamental_size(type);
	
	for (uint32_t i = 0; i < num_elements; )
	{
		offset += to_string__(dst + offset, dst_size - offset, type, p);
		p += sz;

		i++;
		if (i != num_elements)
		{
			if (offset < dst_size)
				dst[offset] = ' ';
			offset++;
		}
	}

	return offset;
}

}
