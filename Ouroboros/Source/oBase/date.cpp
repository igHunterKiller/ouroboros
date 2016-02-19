// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/date.h>
#include <oString/stringize.h>
#include <cassert>
#include <ctime>
#include <calfaq/calfaq.h>

#if _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// NTP v4

using namespace std::chrono;

// Important Dates
static const int kNumSecondsPerDay = 86400;
static const int kNumSecondsPerHour = 3600;
// Documented at http://www.eecis.udel.edu/~mills/database/brief/arch/arch.pdf
// (and it matches calculation from oDateCalcJulianDateNumber)
static const int kNTPEpochJDN = 2415021;
static const int kUnixEpochJDN = 2440588;
static const int kFileTimeEpochJDN = 2305814;
static const int kLastNonGregorianJDN = 2299150;
static const uint64_t kSecondsFrom1900To1970 = 25567ull * kNumSecondsPerDay;
static const uint64_t kSecondsFrom1601To1900 = static_cast<uint64_t>(kNTPEpochJDN - kFileTimeEpochJDN) * kNumSecondsPerDay;

namespace ouro {

#define oDATE_OUT_OF_RANGE(_ToType) do { char buf[1024]; snprintf(buf, sizeof(buf), "date_cast<%s>(const %s&) out of range", typeid(_date).name(), typeid(_ToType).name()); throw std::domain_error(buf); } while(false)
#define oDATE_INVALID_ARG(format, ...) do { char buf[1024]; snprintf(buf, sizeof(buf), format, ## __VA_ARGS__); oThrow(std::errc::invalid_argument, buf); } while(false)

// From NTP reference implementation ntp-dev-4.2.7p295/libntp/ntp_calendar.cpp:
/*
 * Some notes on the terminology:
 *
 * We use the proleptic Gregorian calendar, which is the Gregorian
 * calendar extended in both directions ad infinitum. This totally
 * disregards the fact that this calendar was invented in 1582, and
 * was adopted at various dates over the world; sometimes even after
 * the start of the NTP epoch.
 */
// But this causes the Julian epoch to not be right.

//#define USE_GREGORIAN_ALWAYS

#ifdef USE_GREGORIAN_ALWAYS
	#define IF_SHOULD_USE_GREGORIAN(_oDATE) if (true)
	#define IF_SHOULD_USE_GREGORIAN2(_JDN) if (true)
#else
	//#define IF_SHOULD_USE_GREGORIAN(_date) if (_oDATE > oDATE_JULIAN_EPOCH_END)
	#define IF_SHOULD_USE_GREGORIAN(_oDATE) if (_oDATE > date(-2000, month::January, 1)) // @tony: I have no justification for this choice, but the EPOCH is simply not calculated using Gregorian, but 2 BCE starts failing using Julian...
	#define IF_SHOULD_USE_GREGORIAN2(_JDN) if (_JDN > 730500) // 2000 * 365 + 2000/4
#endif

// _____________________________________________________________________________
// Julian Date Support

int64_t julian_day_number(const date& _date)
{
	if (_date.year < min_julian_valid_date.year || (_date.year == min_julian_valid_date.year && _date.month < min_julian_valid_date.month))
		return -1;

	int style = JULIAN; // 0 = julian, 1 = gregorian
	IF_SHOULD_USE_GREGORIAN(_date)
		style = GREGORIAN;

	return date_to_jdn(style, _date.year, (int)_date.month, (int)_date.day);
}

double julian_date(const date& _date)
{
	int64_t JDN = julian_day_number(_date);
	if (JDN == -1)
		return std::numeric_limits<double>::quiet_NaN();
	return JDN + (_date.hour-12)/24.0 + _date.minute/1440.0 + _date.second/86400.0;
}

double modified_julian_date(const date& _date)
{
	double JD = julian_date(_date);
	if (JD != JD) // where's my std::isnan()?
		return JD;
	return JD - 2400000.5;
}

weekday day_of_week(const date& _date)
{
	int style = JULIAN; // 0 = julian, 1 = gregorian
	IF_SHOULD_USE_GREGORIAN(_date)
		style = GREGORIAN;
	return static_cast<weekday>(::day_of_week(style, abs(_date.year), (int)_date.month, (int)_date.day));
}

static void julian_day_number_to_date(int64_t jdn, date* out_date)
{
	int style = JULIAN;
	IF_SHOULD_USE_GREGORIAN2(jdn)
		style = GREGORIAN;
	jdn_to_date(style, static_cast<int>(jdn), &out_date->year, (int*)&out_date->month, (int*)&out_date->day);
	out_date->day_of_week = static_cast<weekday>(::day_of_week(style, abs(out_date->year), (int)out_date->month, (int)out_date->day));
	out_date->hour = out_date->minute = out_date->second = out_date->millisecond = 0;
}

// ignores year/month/day and returns the number of seconds for _NumDays + hour + minute + second
int64_t calc_num_seconds(int64_t num_days, const date& _date)
{
	return num_days * kNumSecondsPerDay + _date.hour*kNumSecondsPerHour + _date.minute*60 + _date.second; 
}

void calc_hms(int64_t num_seconds, date* out_date)
{
	int64_t s = abs(num_seconds);
	out_date->hour = (s / 3600) % 24;
	out_date->minute = (s / 60) % 60;
	out_date->second = s % 60;
}

// _____________________________________________________________________________
// From date

static bool date_cast(const date& _date, int* out_ntp_era, uint32_t* out_ntp_timestamp)
{
	int64_t JDN = julian_day_number(_date);
	if (JDN == -1)
		return false;
	int64_t DaysIntoEpoch = JDN - kNTPEpochJDN;
	int64_t SecondsIntoEpoch = calc_num_seconds(DaysIntoEpoch, _date);
	// As documented in RFC5905, section 7.1
	*out_ntp_era = static_cast<int>(SecondsIntoEpoch >> 32ll);
	*out_ntp_timestamp = static_cast<uint32_t>((SecondsIntoEpoch - (static_cast<int64_t>(*out_ntp_era) << 32ll)) & 0xffffffff);
	return true;
}

template<> ntp_timestamp date_cast<ntp_timestamp>(const date& _date)
{
	int Era = 0;
	uint32_t Timestamp = 0;
	if (!date_cast(_date, &Era, &Timestamp) || Era != 0)
		oDATE_OUT_OF_RANGE(ntp_timestamp);
	return (static_cast<uint64_t>(Timestamp) << 32) | fractional_second32(duration_cast<fractional_second32>(milliseconds(_date.millisecond))).count();
}

template<> ntp_date date_cast<ntp_date>(const date& _date)
{
	int Era = 0;
	uint32_t Timestamp = 0;
	if (!date_cast(_date, &Era, &Timestamp))
		oDATE_OUT_OF_RANGE(ntp_date);
  uint128_t n((static_cast<uint64_t>(*(uint64_t*)&Era) << 32) | Timestamp, _date.millisecond * (ULLONG_MAX / 1000ull));
	return *(ntp_date*)&n;
}

template<> time_t date_cast<time_t>(const date& _date)
{
	int64_t JDN = julian_day_number(_date);
	if (JDN < kUnixEpochJDN) // should this cap at the upper end? signed? unsigned? 64-bit?
		oDATE_OUT_OF_RANGE(time_t);
	int64_t UnixDays = JDN - kUnixEpochJDN;
	int64_t s = calc_num_seconds(UnixDays, _date);
	if (s > INT_MAX)
		oDATE_OUT_OF_RANGE(time_t);
	return static_cast<time_t>(s);
}

template<> file_time_t date_cast<file_time_t>(const date& _date)
{
	int64_t JDN = julian_day_number(_date);
	if (JDN < kFileTimeEpochJDN) // should this cap at the upper end? signed? unsigned? 64-bit?
		oDATE_OUT_OF_RANGE(file_time_t);
	seconds s(calc_num_seconds(JDN - kFileTimeEpochJDN, _date));
	file_time whole = duration_cast<file_time>(s);
	file_time fractional = duration_cast<file_time>(milliseconds(_date.millisecond));
	return file_time_t((whole + fractional).count());
}

// _____________________________________________________________________________
// From ntp_date

template<> ntp_timestamp date_cast<ntp_timestamp>(const ntp_date& _date)
{
	uint32_t Era = _date.hi() >> 32;
	if (Era != 0)
		oDATE_OUT_OF_RANGE(ntp_timestamp);
	const uint32_t LS = static_cast<uint32_t>(_date.lo() * (UINT_MAX / (double)ULLONG_MAX));
	return (_date.hi() << 32) | LS;
}

template<> ntp_date date_cast<ntp_date>(const ntp_date& _date)
{
	return _date;
}

template<> time_t date_cast<time_t>(const ntp_date& _date)
{
	int Era = get_ntp_era(_date);
	if (Era != 0)
		oDATE_OUT_OF_RANGE(time_t);
	int64_t s = get_ntp_seconds(_date);
	if (s < kSecondsFrom1900To1970)
		oDATE_OUT_OF_RANGE(time_t);
	return s - kSecondsFrom1900To1970;
}

template<> file_time_t date_cast<file_time_t>(const ntp_date& _date)
{
	int Era = get_ntp_era(_date);
	if (Era < -3) // the era when 1601 is
		oDATE_OUT_OF_RANGE(file_time_t);
	seconds s(get_ntp_seconds(_date) + kSecondsFrom1601To1900);
	if (s.count() < 0)
		oDATE_OUT_OF_RANGE(file_time_t);
	file_time whole = duration_cast<file_time>(s);
	file_time fractional(file_time_ratio::den * (_date.lo() / ULLONG_MAX));
	return file_time_t((whole + fractional).count());
}

template<> date date_cast<date>(const ntp_date& _date)
{
	int64_t s = get_ntp_seconds(_date);
	int64_t JDN = (s / kNumSecondsPerDay) + kNTPEpochJDN;
	date d;
	julian_day_number_to_date(JDN, &d);
	calc_hms(s, &d);
	d.millisecond = static_cast<int>(1000 * (_date.lo() / ULLONG_MAX));
	return d;
}

template<> date date_cast<date>(const date& _date)
{
	// force a cleanup of the data if they are out-of-range
	ntp_date d = date_cast<ntp_date>(_date);
	return date_cast<date>(d);
}

// _____________________________________________________________________________
// From ntp_timestamp

template<> ntp_timestamp date_cast<ntp_timestamp>(const ntp_timestamp& _date)
{
	return _date;
}

template<> ntp_date date_cast<ntp_date>(const ntp_timestamp& _date)
{
	fractional_second32 fractional32(_date & ~0u);
  uint128_t n(static_cast<uint32_t>(_date >> 32ull), fractional32.count() * (ULLONG_MAX / UINT_MAX));
	return *(ntp_date*)&n;
}

template<> time_t date_cast<time_t>(const ntp_timestamp& _date)
{
	uint32_t s = _date >> 32ull;
	if (s < kSecondsFrom1900To1970)
		oDATE_OUT_OF_RANGE(time_t);
	return _date - kSecondsFrom1900To1970;
}

template<> file_time_t date_cast<file_time_t>(const ntp_timestamp& _date)
{
	file_time whole = duration_cast<file_time>(seconds((_date >> 32ull) + kSecondsFrom1601To1900));
	file_time fractional = duration_cast<file_time>(fractional_second32(_date & ~0u));
	return file_time_t((whole + fractional).count());
}

template<> date date_cast<date>(const ntp_timestamp& _date)
{
	uint32_t s = get_ntp_timestamp(_date);
	int64_t JDN = (s / kNumSecondsPerDay) + kNTPEpochJDN;
	date d;
	julian_day_number_to_date(JDN, &d);
	calc_hms(s, &d);
	d.millisecond = static_cast<int>(duration_cast<milliseconds>(fractional_second32(_date & ~0u)).count());
	return d;
}

// _____________________________________________________________________________
// From ntp_time

template<> ntp_timestamp date_cast<ntp_timestamp>(const ntp_time& _date)
{
	uint64_t sec = _date >> 16;
	return (sec << 32) | duration_cast<fractional_second32>(fractional_second16(_date & 0xffff)).count();
}

template<> ntp_date date_cast<ntp_date>(const ntp_time& _date)
{
	fractional_second16 fractional16(_date & 0xffff);
  uint128_t n(_date >> 16, fractional16.count() * (ULLONG_MAX / USHRT_MAX));
	return *(ntp_date*)&n;
}

template<> time_t date_cast<time_t>(const ntp_time& _date)
{
	return _date >> 16;
}

template<> file_time_t date_cast<file_time_t>(const ntp_time& _date)
{
	file_time whole = duration_cast<file_time>(seconds(_date >> 16));
	file_time fractional = duration_cast<file_time>(fractional_second16(_date & 0xffff));
	whole += fractional;
	if (whole.count() < 0)
		oDATE_OUT_OF_RANGE(file_time_t);
	return whole.count();
}

template<> date date_cast<date>(const ntp_time& _date)
{
	// ntp prime epoch
	date d;
	d.year = 1900;
	d.month = month::January;
	d.day = 1;
	calc_hms((_date >> 16), &d);
	d.millisecond = static_cast<int>(duration_cast<milliseconds>(fractional_second16(_date & 0xffff)).count());
	return d;
}

// _____________________________________________________________________________
// From time_t

template<> ntp_timestamp date_cast<ntp_timestamp>(const time_t& _date)
{
	int64_t s = static_cast<uint32_t>(_date + kSecondsFrom1900To1970);
	return s << 32;
}

template<> ntp_date date_cast<ntp_date>(const time_t& _date)
{
	uint128_t n(_date + kSecondsFrom1900To1970, 0);
	return *(ntp_date*)&n;
}

template<> time_t date_cast<time_t>(const time_t& _date)
{
	return _date;
}

template<> file_time_t date_cast<file_time_t>(const time_t& _date)
{
	return file_time_t(duration_cast<file_time>(seconds(_date + kSecondsFrom1601To1900 + kSecondsFrom1900To1970)).count());
}

template<> date date_cast<date>(const time_t& _date)
{
	int64_t JDN = (_date / kNumSecondsPerDay) + kUnixEpochJDN;
	date d;
	julian_day_number_to_date(JDN, &d);
	calc_hms(_date, &d);
	d.millisecond = 0;
	return d;
}

// _____________________________________________________________________________
// From file_time_t

template<> ntp_timestamp date_cast<ntp_timestamp>(const file_time_t& _date)
{
	int64_t usec = (int64_t)_date / 10;
	fractional_second32 fractional = duration_cast<fractional_second32>(microseconds(usec % std::micro::den));
	int64_t s = (usec / std::micro::den) - kSecondsFrom1601To1900;
	if (s < 0 || ((s & ~0u) != s))
		oDATE_OUT_OF_RANGE(ntp_timestamp);
	return (s << 32) | fractional.count();
}

template<> ntp_date date_cast<ntp_date>(const file_time_t& _date)
{
	int64_t usec = (int64_t)_date / 10;
	uint128_t n((usec / std::micro::den) - kSecondsFrom1601To1900, file_time((int64_t)_date % file_time::period::den).count() * (ULLONG_MAX / file_time_ratio::den));
	return *(ntp_date*)&n;
}

template<> time_t date_cast<time_t>(const file_time_t& _date)
{
	int64_t s = duration_cast<seconds>(microseconds((int64_t)_date / 10)).count() - kSecondsFrom1601To1900 - kSecondsFrom1900To1970;
	if (s < 0 || s > INT_MAX)
		oDATE_OUT_OF_RANGE(time_t);
	return static_cast<int>(s);
}

template<> file_time_t date_cast<file_time_t>(const file_time_t& _date)
{
	return _date;
}

template<> date date_cast<date>(const file_time_t& _date)
{
	ntp_date d = date_cast<ntp_date>(_date);
	return date_cast<date>(d);
}

// _____________________________________________________________________________
// to FILETIME

#ifdef _MSC_VER
	template<> ntp_timestamp date_cast<ntp_timestamp>(const FILETIME& _date) { return date_cast<ntp_timestamp>((const file_time_t&)_date); }
	template<> ntp_date date_cast<ntp_date>(const FILETIME& _date) { return date_cast<ntp_date>((const file_time_t&)_date); }
	template<> time_t date_cast<time_t>(const FILETIME& _date) { return date_cast<time_t>((const file_time_t&)_date); }
	template<> file_time_t date_cast<file_time_t>(const FILETIME& _date) { return date_cast<file_time_t>((const file_time_t&)_date); }
	template<> date date_cast<date>(const FILETIME& _date) { return date_cast<date>((const file_time_t&)_date); }

	template<> FILETIME date_cast<FILETIME>(const date& _date) { file_time_t t = date_cast<file_time_t>(_date); return *(FILETIME*)&t; }
	template<> FILETIME date_cast<FILETIME>(const ntp_date& _date) { file_time_t t = date_cast<file_time_t>(_date); return *(FILETIME*)&t; }
	template<> FILETIME date_cast<FILETIME>(const ntp_timestamp& _date) { file_time_t t = date_cast<file_time_t>(_date); return *(FILETIME*)&t; }
	template<> FILETIME date_cast<FILETIME>(const ntp_time& _date) { file_time_t t = date_cast<file_time_t>(_date); return *(FILETIME*)&t; }
	template<> FILETIME date_cast<FILETIME>(const time_t& _date) { file_time_t t = date_cast<file_time_t>(_date); return *(FILETIME*)&t; }
	template<> FILETIME date_cast<FILETIME>(const file_time_t& _date) { file_time_t t = date_cast<file_time_t>(_date); return *(FILETIME*)&t; }
#endif

// Returns the number of seconds to add to a UTC time to get the time in the 
// current locale's timezone.
static int timezone_offset()
{
	time_t now = time(nullptr);
	tm utc;
	gmtime_s(&utc, &now);
	// get whether we're locally in daylight savings time
	tm local;
	localtime_s(&local, &now);
	bool IsDaylightSavings = !!local.tm_isdst;
	time_t utctimeInterpretedAsLocaltime = mktime(&utc);
	return static_cast<int>(now - utctimeInterpretedAsLocaltime) + (IsDaylightSavings ? kNumSecondsPerHour : 0);
}

size_t strftime(char* dst, size_t dst_size, const char* format, const ntp_date& _date, const date_conversion& conversion)
{
	ntp_date DateCopy(_date);

	int TimezoneOffsetSeconds = timezone_offset();

	if (conversion == date_conversion::to_local)
		DateCopy.hi(DateCopy.hi() + TimezoneOffsetSeconds);

	date d = date_cast<date>(DateCopy); // clean up any overrun values
	attoseconds fractional(static_cast<int64_t>(_date.lo() >> 1ull));

	*dst = 0;
	char* w = dst;
	char* wend = dst + dst_size;
	const char* f = format;
	char s[128];
	*s = '\0';
	size_t len = 0;

#define ADD_CHECK() do \
	{	if (!len) \
	return 0; \
	w += len; \
	} while(false)

#define ADDS() do \
	{	len = snprintf(w, std::distance(w, wend), "%s", s); \
	ADD_CHECK(); \
	} while(false)

#define ADDSTR(_String) do \
	{	len = snprintf(w, std::distance(w, wend), (_String)); \
	ADD_CHECK(); \
	} while(false)

#define ADDDIG(n) do \
	{	len = snprintf(w, std::distance(w, wend), "%u", (n)); \
	ADD_CHECK(); \
	} while(false)

#define ADD2DIG(n) do \
	{	len = snprintf(w, std::distance(w, wend), "%02u", (n)); \
	ADD_CHECK(); \
	} while(false)

#define ADDDATE(format) do \
	{	len = strftime(w, std::distance(w, wend), format, _date, conversion); \
	ADD_CHECK(); \
	} while(false)

#define ADDDUR(durationT) do \
	{ durationT d = duration_cast<durationT>(fractional); \
	len = snprintf(w, std::distance(w, wend), "%llu", d.count()); \
	ADD_CHECK(); \
	} while(false)

#define ADDDUR0(pad, durationT) do \
	{ durationT d = duration_cast<durationT>(fractional); \
	len = snprintf(w, std::distance(w, wend), "%0" #pad "llu", d.count()); \
	ADD_CHECK(); \
	} while(false)

	while (*f)
	{
		if (*f == '%')
		{
			f++;
			switch (*f)
			{
				case 'a': strcpy_s(s, as_string(d.day_of_week)); s[3] = 0; ADDS(); break;
				case 'A': ADDSTR(as_string(d.day_of_week)); break;
				case 'b': strcpy_s(s, as_string(d.month)); s[3] = 0; ADDS(); break;
				case 'B': ADDSTR(as_string(d.month)); break;
				case 'c': ADDDATE("%a %b %d %H:%M:%S %Y"); break;
				case 'd': ADD2DIG(d.day); break;
				case 'H': ADD2DIG(d.hour); break;
				case 'I': ADD2DIG(d.hour % 12); break;
				case 'j': assert(0 && "Day of year not yet supported"); break;
				case 'm': ADD2DIG(d.month); break;
				case 'M': ADD2DIG(d.minute); break;
				case 'o': 
				{
					int Hours = TimezoneOffsetSeconds / 3600;
					int Minutes = (abs(TimezoneOffsetSeconds) / 60) - abs(Hours * 60);
					len = snprintf(w, std::distance(w, wend), "%+d:%02d", Hours, Minutes);
					ADD_CHECK();
					break;
				}

				case 'p': ADDSTR(d.hour < 12 ? "AM" : "PM"); break;
				case 'S': ADD2DIG(d.second); break;
				case 'U': assert(0 && "Week of year not yet supported"); break;
				case 'w': ADDDIG(d.day_of_week); break;
				case 'W': assert(0 && "Week of year not yet supported"); break;
				case 'x': ADDDATE("%m/%d/%y"); break;
				case 'X': ADDDATE("%H:%M:%S"); break;
				case 'y': ADD2DIG(d.year % 100); break;
				case 'Y':
				{
					if (d.year >= 1)
						ADDDIG(d.year);
					else
					{
						ADDDIG(abs(--d.year));
						ADDSTR(" BCE");
					}
					break;
				}

				case 'Z':
				{
					time_t t;
					time(&t);
					tm TM;
					localtime_s(&TM, &t);
					len = ::strftime(dst, std::distance(w, wend), "%Z", &TM);
					ADD_CHECK();
					break;
				}

				case '%': *w++ = *f++; break;
				// Non-standard fractional seconds
				case 's': ADDDUR(milliseconds); break;
				case 'u': ADDDUR(microseconds); break;
				case 'P': ADDDUR(picoseconds); break;
				case 't': ADDDUR(attoseconds); break;

				case '0':
				{
					f++;
					switch (*f)
					{
						case 's': ADDDUR0(3, milliseconds); break;
						case 'u': ADDDUR0(6, microseconds); break;
						default:
							oDATE_INVALID_ARG("not supported: formatting token %%0%c", *f);
					}

					break;
				}

				default:
					oDATE_INVALID_ARG("not supported: formatting token %%%c", *f);
			}

			f++;
		}

		else
			*w++ = *f++;
	}

	*w = 0;
	return std::distance(dst, w);
}

char* to_string(char* dst, size_t dst_size, const date& _date)
{
	return strftime(dst, dst_size, sortable_date_format, date_cast<ntp_date>(_date)) ? dst : nullptr;
}

char* to_string(char* dst, size_t dst_size, const weekday& w)
{
	return strlcpy(dst, as_string(w), dst_size) < dst_size ? dst : nullptr;
}

char* to_string(char* dst, size_t dst_size, const month& m)
{
	return strlcpy(dst, as_string(m), dst_size) < dst_size ? dst : nullptr;
}

bool from_string(weekday* out_value, const char* src)
{
	for (size_t i = 0; i < 7; i++)
	{
		const char* s = as_string((weekday)i);
		if (!_stricmp(src, s) || (!_memicmp(src, s, 3) && src[3] == 0))
		{
			*out_value = (weekday)i;
			return true;
		}
	}
	return false;
}

bool from_string(month* out_value, const char* src)
{
	for (size_t i = 0; i < 12; i++)
	{
		const char* s = as_string((month)i);
		if (!_stricmp(src, s) || (!_memicmp(src, s, 3) && src[3] == 0))
		{
			*out_value = (month)i;
			return true;
		}
	}
	return false;
}

}
