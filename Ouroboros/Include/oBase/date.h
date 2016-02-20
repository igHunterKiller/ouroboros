// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Support Date/Time/Wall Clock functionality in a way that should really be the 
// final statement on the matter. NTPv4 has attosecond accuracy while supporting 
// multiple 136-year eras. This header also supports legacy formats: time_t, 
// file_time_t and the human-workable 'date' format.
// NTP v4: http://tools.ietf.org/html/rfc5905#section-6

#pragma once
#include <oCore/stringize.h> // as_string
#include <oCore/uint128.h>
#include <chrono>
#include <climits>
#include <string>

namespace ouro {

// _____________________________________________________________________________
// Basic date types

// NTP short format timestamp, use for small deltas (prefer a millisecond timer instead)
typedef uint32_t ntp_time;

// NTPv1/v2 medium-format timestamp, use when time_t or file_time (FILETIME) might be used
typedef uint64_t ntp_timestamp;

// long-format timestamp, use for either attosecond accuracy or dates far into the fast or future
struct ntp_date : uint128 {};

// NTPv4 fixed-point representation of a portion of a second in units of about 232 picoseconds 
// NOTE: fractional_second64 would be units of 0.05 attoseconds, but std::ratio uses INT_MAX, 
// not UINTMAX it cannot be represented and thus is not defined here
typedef std::chrono::duration<uint16_t, std::ratio<1,USHRT_MAX>> fractional_second16;
typedef std::chrono::duration<uint32_t, std::ratio<1,UINT_MAX>> fractional_second32;
typedef std::chrono::duration<int64_t, std::pico> picoseconds;
typedef std::chrono::duration<int64_t, std::atto> attoseconds;
typedef std::chrono::duration<double> secondsd;

// this is in 100 nanosecond units as is used in FILETIME
typedef std::ratio<1,10000000> file_time_ratio;
typedef std::chrono::duration<int64_t, file_time_ratio> file_time;

class file_time_t // FILETIME on windows
{
public:
	file_time_t() {}
	file_time_t(int64_t num_100_nanosec_intervals)
		: lo_(num_100_nanosec_intervals & ~0u)
		, hi_(num_100_nanosec_intervals >> 32ll)
	{}

	operator int64_t() const
	{
		uint64_t ull = (static_cast<uint64_t>(hi_) << 32) | lo_;
		return *(int64_t*)&ull;
	}

private:
	uint32_t lo_;
	uint32_t hi_;
};

enum class weekday { Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Unknown, };
enum class month { January = 1, February, March, April, May, June, July, August, September, October, November, December, };

template<> inline const char* as_string<weekday>(const weekday& _weekday)
{
	static const char* sWeekdayStrings[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
	return sWeekdayStrings[(int)_weekday];
}

template<> inline const char* as_string<month>(const month& _month)
{
	static const char* sMonthStrings[13] = { "invalid", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
	return sMonthStrings[(int)_month];
}

// This calculates dates using the proleptic Gregorian calendar method - i.e.
// where all dates, even those before the Gregorian calendar was invented, are 
// calculated using the Gregorian method. This is consistent with how NTP 
// reports time.
class date
{
public:
	date(int _year = 0, month _month = month::January, int _day = 0, int _hour = 0, int _minute = 0, int _second = 0, int _millisecond = 0)
		: year(_year)
		, month(_month)
		, day_of_week(weekday::Unknown)
		, day(_day)
		, hour(_hour)
		, minute(_minute)
		, second(_second)
		, millisecond(_millisecond)
	{}

	int year; // The full astronomical year. (1 CE == 1, 1 BCE == 0, 2 BCE = -1, 4713 BCE = -4712)
	month month;
	weekday day_of_week; // this is ignored when date is a source for date_cast
	int day; // [1,31]
	int hour; // [0,23]
	int minute; // [0,59]
	int second; // [0,59] We don't support leap seconds because the standards don't
	int millisecond; // [0,999]

	inline bool operator<(const date& that) const
	{
		return year < that.year
			|| (year == that.year && month <  that.month)
			|| (year == that.year && month == that.month && day <  that.day)
			|| (year == that.year && month == that.month && day == that.day && hour <  that.hour)
			|| (year == that.year && month == that.month && day == that.day && hour == that.hour && minute <  that.minute)
			|| (year == that.year && month == that.month && day == that.day && hour == that.hour && minute == that.minute && second <  that.second)
			|| (year == that.year && month == that.month && day == that.day && hour == that.hour && minute == that.minute && second == that.second && millisecond < that.millisecond);
	}

	inline bool operator> (const date& that) const { return (that < *this); }
	inline bool operator<=(const date& that) const { return !(*this > that); }
	inline bool operator>=(const date& that) const { return !(*this < that); }
	inline bool operator==(const date& that) const { return year == that.year && month == that.month && day == that.day && hour == that.hour && minute == that.minute && second == that.second && millisecond == that.millisecond; }
	inline bool operator!=(const date& that) const { return !(*this == that); }
};

static const date min_julian_valid_date(-4800, month::March, 1, 12);
static const date julian_epoch_start(-4712, month::January, 1, 12);
static const date julian_epoch_end(1582, month::October, 4);
static const date gregorian_epoch_start(1582, month::October, 15);
static const date gregorian_epoch_start_england_sweden(1752, month::September, 2);
static const date ntp_epoch_start(1900, month::January, 1);
static const date ntp_epoch_end(2036, month::February, 7, 6, 28, 14);
static const date unix_time_epoch_start(1970, month::January, 1);
static const date unix_time_epoch_end_signed32(2038, month::January, 19, 3, 14, 7);
static const date unix_time_epoch_end_unsigned32(2106, month::February, 7, 6, 28, 15);
static const date file_time_epoch_start(1601, month::January, 1);
//static const date file_time_epoch_end(?);

// Returns the Julian Day Number (JDN), (i.e. Days since noon on the first day 
// of recorded history: Jan 1, 4713 BCE. This function interprets on or after 
// Oct 15, 1582 as a Gregorian Calendar date and dates prior as a Julian 
// Calendar Date. This will not return correct values for dates before 
// March 1, 4801 BCE and will instead return ouro::invalid.
int64_t julian_day_number(const date& d);

// Returns the Julian Date (JD) from the specified date. This begins with 
// calculation of the Julian Day Number, so if that is invalid, this will return
// NaN.
double julian_date(const date& d);

// Returns the Modified Julian Date (MJD) from the specified date. This begins
// with calculation of the Julian Day Number, so if that is invalid, this will 
// return NaN.
double modified_julian_date(const date& d);

// Returns the day of the week for the specified date
weekday day_of_week(const date& d);

// Returns the component of an ntp_date according to NTPv4.
inline int64_t get_ntp_date(const ntp_date& d) { auto hi = d.hi(); return *(int64_t*)&hi; }
inline int get_ntp_era(const ntp_date& d) { uint32_t i = d.hi() >> 32; return *(int*)&i; }
inline uint32_t get_ntp_timestamp(const ntp_date& d) { return d.hi() & ~0u; }
inline double get_ntp_fractional_seconds(const ntp_date& d) { return d.lo() / (double)ULLONG_MAX; }
inline int64_t get_ntp_seconds(const ntp_date& d) { int64_t era = get_ntp_era(d); return (era << 32) + get_ntp_timestamp(d); }
inline uint32_t get_ntp_timestamp(const ntp_timestamp& timestamp) { return timestamp >> 32; }
inline double get_ntp_fractional_seconds(const ntp_timestamp& timestamp) { return (timestamp & ~0u) / (double)UINT_MAX; }

// cast from one date type to another
template<typename DateT1, typename DateT2> DateT1 date_cast(const DateT2& d);

// Common date formats for use with strftime

static const char* http_date_format = "%a, %d %b %Y %H:%M:%S GMT"; // Fri, 31 Dec 1999 23:59:59 GMT

// Sortable in a text output
static const char* sortable_date_format = "%Y/%m/%d %X"; // 1999/12/31 23:59:59
static const char* sortable_date_ms_format = "%Y/%m/%d %X:%0s"; // 1999/12/31 23:59:59:325
static const char* short_date_format = "%m/%d %H:%M"; // 12/31 23:59

// The RFC 5424 formats: http://tools.ietf.org/html/rfc5424#section-6.1
static const char* syslog_utc_date_format = "%Y-%m-%dT%X.%0sZ"; // 2003-10-11T22:14:15.003Z
static const char* syslog_local_date_format = "%Y-%m-%dT%X.%0s%o"; // 2003-08-24T05:14:15.000003-07:00

enum class date_conversion { none, to_local };

// Same syntax and usage as strftime, with the added formatters:
// %o: +-H:M offset from UTC time.
// %s: milliseconds. or %0s, meaning pad to 3 digits (i.e. 003)
// %u: microseconds. or %0u, meaning pad to 6 digits (i.e. 000014)
// %P: picoseconds.
// %t: attoseconds.
size_t strftime(char* dst, size_t dst_size, const char* format, const ntp_date& _date, const date_conversion& conversion = date_conversion::none);

template<typename dateT>
size_t strftime(char* dst, size_t dst_size, const char* format, const dateT& _date, const date_conversion& conversion = date_conversion::none)
{
	return strftime(dst, dst_size, format, date_cast<ntp_date>(_date), conversion);
}

template<typename dateT, size_t size>
size_t strftime(char (&dst)[size], const char* format, const dateT& _date, const date_conversion& conversion = date_conversion::none)
{
	return strftime(dst, size, format, _date, conversion);
}

}

namespace std {

inline std::string to_string(const ouro::weekday& w) { return ouro::as_string(w); }
inline std::string to_string(const ouro::month& m) { return ouro::as_string(m); }

}
