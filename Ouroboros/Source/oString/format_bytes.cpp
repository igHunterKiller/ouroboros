// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <math.h>
#include <float.h>

#define KB(n) (   n  * 1024LL)
#define MB(n) (KB(n) * 1024LL)
#define GB(n) (MB(n) * 1024LL)
#define TB(n) (GB(n) * 1024LL)

int ouro::format_bytes(char* dst, size_t dst_size, uint64_t bytes, size_t num_precision_digits)
{
	char fmt[16];
	int result = snprintf(fmt, "%%.0%uf %%s%%s", num_precision_digits);

	const char* type = "";
	double amount = 0.0;

	#define ELIF(label) else if (bytes > label(1)) { type = #label; amount = bytes / static_cast<double>(label(1)); }
		if (bytes < KB(1)) { type = "byte"; amount = static_cast<double>(bytes); }
		ELIF(TB) ELIF(GB) ELIF(MB) ELIF(KB)
	#undef ELIF

	const char* plural = (abs(amount - 1.0) < DBL_EPSILON) ? "" : "s";
	return snprintf(dst, dst_size, fmt, amount, type, plural);
}
