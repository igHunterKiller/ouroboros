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

int snprintf(char* dst, size_t dst_size, const fundamental& type, uint32_t num_elements, const void* src)
{
	#define PR(fmt, type) do { size_t maxn = dst_size - offset; int n = snprintf(dst + offset, maxn, fmt, *(const type*)p); if (n < 0) return -1; p += sizeof(type); offset += n; } while (false)

	int offset = 0;
	auto p = (const char*)src;
	for (uint32_t i = 0; i < num_elements; )
	{
		switch (type)
		{
			case fundamental::bool_type: { size_t maxn = dst_size - offset; int n = snprintf(dst, dst_size, "%s", *(const bool*)src ? "true" : "false"); if (n < 0) return -1; p += sizeof(bool); offset += n; break; }
			case fundamental::char_type:   PR("%c", char);                 break;
			case fundamental::uchar_type:  PR("%u", unsigned char);        break;
			case fundamental::wchar_type:  PR("%lc", wchar_t);             break;
			case fundamental::short_type:  PR("%d", short);                break;
			case fundamental::ushort_type: PR("%u", unsigned short);       break;
			case fundamental::int_type:
			case fundamental::long_type:   PR("%d", int);                  break;
			case fundamental::uint_type:
			case fundamental::ulong_type:  PR("%u", int);                  break;
			case fundamental::llong_type:  PR("%lld", long long);          break;
			case fundamental::ullong_type: PR("%llu", unsigned long long); break;
			case fundamental::float_type:  PR("%f", float);                break;
			case fundamental::double_type: PR("%f", double);               break;
			default: return -1;
		}

		i++;
		if (i != num_elements)
		{
			if (offset < dst_size)
				dst[offset++] = ' ';
			if (offset < dst_size)
				dst[offset] = '\0';
		}
	}

	return offset;
}

}
