// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <stdexcept>
#include <cmath>

#include <oCore/assert.h>

static inline const char* plural(unsigned int n) { return n == 1 ? "" : "s"; }

static int cat_part(char* dst, size_t dst_size, unsigned int _Amount, const char* _Label, const char* abbreviatedLabel, bool _First, bool abbreviated)
{
	return _Amount
		? ouro::sncatf(dst, dst_size, "%s%u%s%s%s"
			, _First ? "" : " "
			, _Amount, abbreviated ? "" : " "
			, abbreviated ? abbreviatedLabel : _Label
			, !abbreviated ? plural(_Amount) : "")

		: static_cast<int>(strlen(dst ? dst : ""));
}

int ouro::format_duration(char* dst, size_t dst_size, double seconds, bool abbreviated, bool include_milliseconds)
{
	if (seconds != seconds) // check for denorm/inf
		seconds = 0.0;

	if (seconds < 0.0)
		throw std::range_error("negative time (did you do start - end instead of end - start?)");

	int result = 0;

	const static double ONE_MINUTE = 60.0;
	const static double ONE_HOUR = 60.0 * ONE_MINUTE;
	const static double ONE_DAY = 24.0 * ONE_HOUR;
	const static double ONE_WEEK = 7 * ONE_DAY;
	const static double ONE_MONTH = 30.0 * ONE_DAY;
	const static double ONE_YEAR = 365 * ONE_DAY;

	#define SHAVE(_Var, _Amt) \
		unsigned int _Var = static_cast<unsigned int>(seconds / (_Amt)); \
		if (_Var) seconds -= ((_Amt) * (_Var));

	SHAVE(year, ONE_YEAR);
	SHAVE(month, ONE_MONTH);
	SHAVE(week, ONE_WEEK);
	SHAVE(day, ONE_DAY);
	SHAVE(hour, ONE_HOUR);
	SHAVE(minute, ONE_MINUTE);
	SHAVE(second, 1);
	unsigned int millisecond = static_cast<unsigned int>((seconds - floor(seconds)) * 1000.0);

 	int len = 0;
	char* s = dst;
	size_t n = dst_size;
	*s = 0;

	#define CAT(_Var, _Abbrev) do \
	{	len = cat_part(s, n, _Var, #_Var, _Abbrev, len == 0, abbreviated); \
		if (static_cast<size_t>(len) >= dst_size) \
		{	s = nullptr; \
			n = 0; \
		} \
	} while (false)

	CAT(year, "y");
	CAT(month, "mo");
	CAT(week, "w");
	CAT(day, "d");
	CAT(hour, "h");
	CAT(minute, "m");
	CAT(second, "s");
	if (include_milliseconds)
		CAT(millisecond, "ms");

	if (!len)
		len = sncatf(dst, dst_size, "0%s", abbreviated ? "s" : " seconds");

	return len;
}
