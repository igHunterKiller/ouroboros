// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oBase/date.h>
#include <oCore/assert.h>
#include <oCore/countof.h>
#include <oCore/macros.h>

template<typename T> T round(const T& x) { return floor(x + T(0.5)); }

using namespace ouro;

struct oNTPDATE_TEST
{
	date Date;
	long long JulianDay;
	long long ModifiedJulianDay;
	long long NTPDate;
	int NTPEra;
	unsigned int NTPTimestamp;
	time_t UnixTime;
	file_time_t FileTime;
	const char* Epoch;
	const char* TimeFormat;
	const char* ExpectedTimeString;
};

struct oDATE_TEST
{
	date Date;
	int dummy;
};

// http://www.eecis.udel.edu/~mills/database/reports/ntp4/ntp4.pdf
// FILETIMEs from http://www.silisoftware.com/tools/date.php
// Table 2. Interesting Historic NTP Dates
static const oNTPDATE_TEST sNTPTests[] = 
{
	{ date(-4712, month::January, 1), 0, -2400001, -208657814400, -49, 1795583104, 0, 0, "First day Julian Era (start of day, epoch is at noon)", "%c", "Sun Jan 01 00:00:00 4713 BCE" },
	{ date(-1, month::January, 1), 1720695, -679306, -59989766400, -14, 139775744, 0, 0, "2 BCE", "%c", "Mon Jan 01 00:00:00 2 BCE" },
	{ date(0, month::January, 1), 1721060, -678941, -59958230400, -14, 171311744, 0, 0, "1 BCE", "%c", "Sun Jan 01 00:00:00 1 BCE" },
	{ date(1, month::January, 1), 1721426, -678575, -59926608000, -14, 202934144, 0, 0, "1 CE", "%c", "Mon Jan 01 00:00:00 1" },
	{ date(1452, month::September, 30), 2251665, -148327 - 9, -14113958400, -4, 3065910784, 0, 0, "First publishing of Guttenberg Bible", "%c", "Thu Sep 30 00:00:00 1452" }, // the minus 9 is for the same reason as HACK_ADJUSTMENT
	{ date(1492, month::August, 3), 2266217, -133775 - 9, -12856665600, -3, 28236288, 0, 0, "Columbus sailed the ocean blue", "%c", "Wed Aug 03 00:00:00 1492" }, // the minus 9 is for the same reason as HACK_ADJUSTMENT
	{ julian_epoch_end, 2299150, -100851, -10011254400, -3, 2873647488, 0, 0, "Last day of Julian Calendar", "%c", "Mon Oct 04 00:00:00 1582" },
	{ gregorian_epoch_start, 2299161, -100840, -10010304000, -3, 2874597888, 0, 0, "First day Gregorian Calendar", "%c", "Fri Oct 15 00:00:00 1582" },
	{ file_time_epoch_start, 2305814, -94187, -9435484800, -3, 3449417088, 0, 0, "First day FILETIME", "%c", "Mon Jan 01 00:00:00 1601" },
	{ date(1899, month::December, 31), 2415020, 15019, -86400, -1, 4294880896, 0, 94353984000000000, "Last day NTP Era -1", "%c", "Sun Dec 31 00:00:00 1899" },
	{ ntp_epoch_start, 2415021, 15020, 0, 0, 0, 0, 94354848000000000, "First day NTP Era 0", "%c", "Mon Jan 01 00:00:00 1900" },
	{ unix_time_epoch_start, 2440588, 40587, 2208988800, 0, 2208988800, 0, 116444736000000000, "First day Unix", "%c", "Thu Jan 01 00:00:00 1970" },
	{ date(1972, month::January, 1), 2441318, 41317, 2272060800, 0, 2272060800, 63072000, 117075456000000000, "First day UTC", "%c", "Sat Jan 01 00:00:00 1972" },
	{ date(1999, month::December, 31), 2451544, 51543, 3155587200, 0, 3155587200, 946598400, 125910720000000000, "Last day 20th century", "%c", "Fri Dec 31 00:00:00 1999" },
	{ date(2000, month::January, 1), 2451545, 51544, 3155673600, 0, 3155673600, 946684800, 125911584000000000, "First day 21st century", "%c", "Sat Jan 01 00:00:00 2000" },
	{ date(2001, month::September, 9, 1, 46, 40), 2452162, 52161, 3208988800, 0, 3208988800, 1000000000, 126444736000000000, "1 Bil Unix Time", "%c", "Sun Sep 09 01:46:40 2001" },
	{ date(2036, month::February, 7), 2464731, 64730, 4294944000, 0, 4294944000, 2085955200, 137304288000000000, "Last day NTP Era 0", "%c", "Thu Feb 07 00:00:00 2036" },
	{ ntp_epoch_end, 2464731, 64730, 4294967294, 0, 4294967294, 2085978494, 137304520940000000, "Last moment NTP Era 0", "%c", "Thu Feb 07 06:28:14 2036" },
	{ date(2036, month::February, 8), 2464732, 64731, 4295030400, 1, 63104, 2086041600, 137305152000000000, "First day NTP Era 1", "%c", "Fri Feb 08 00:00:00 2036" },
	{ unix_time_epoch_end_signed32, 2465443, 65442, 4356472447, 1, 61505151, 2147483647, 137919572470000000, "Last day signed 32-bit Unix", "%c", "Tue Jan 19 03:14:07 2038" },
	{ unix_time_epoch_end_unsigned32, 2490298, 90297, 6503956095, 1, 2208988799, 4294967296, 159394408950000000, "Last day unsigned 32-bit Unix", "%c", "Sun Feb 07 06:28:15 2106" },
	{ date(2172, month::March, 16), 2514442, 114441, 8589974400, 2, 39808, 6380985600, 180254592000000000, "First day NTP Era 2", "%c", "Mon Mar 16 00:00:00 2172" },
	{ date(2500, month::January, 1), 2634167, 234166, 18934214400, 4, 1754345216, 0, 283696992000000000, "2500 CE", "%c", "Fri Jan 01 00:00:00 2500" },
	{ date(3000, month::January, 1), 2816788, 416787, 34712668800, 8, 352930432, 0, 441481536000000000, "3000 CE", "%c", "Wed Jan 01 00:00:00 3000" },
	{ date(2005, month::May, 24), 2453515, 53514, 3325881600, 0, 3325881600, 1116892800, 127613664000000000, "http://www.mathworks.com/help/toolbox/aerotbx/ug/mjuliandate.html", "%c", "Tue May 24 00:00:00 2005" },
	{ date(2006, month::December, 19), 2454089, 54088, 3375475200, 0, 3375475200, 1166486400, 128109600000000000, "http://www.mathworks.com/help/toolbox/aerotbx/ug/mjuliandate.html", "%c", "Tue Dec 19 00:00:00 2006" },
	{ date(2004, month::October, 10, 12, 21), 2453289, 53288, 3306399660, 0, 3306399660, 1097410860, 127418844600000000, "http://www.mathworks.com/help/toolbox/aerotbx/ug/mjuliandate.html", "%c", "Sun Oct 10 12:21:00 2004" },
	{ date(2012, month::August, 15, 15, 12, 58), 2456155, 56154, 3554032378, 0, 3554032378, 1345043578, 129895171780000000, "Day this test was written", "%c", "Wed Aug 15 15:12:58 2012" },
	{ date(2012, month::August, 12, 23, 21, 36), 2456152, 56151, 3553802496, 0, 3553802496, 1344813696, 129892872960000000, "http://en.wikipedia.org/wiki/Unix_time", "%c", "Sun Aug 12 23:21:36 2012" },
};

static void throw_failed_date(const char* _DateName, const char* _DateType, long long _ExpectedValue, long long _CalculatedValue)
{
	throw std::system_error(std::errc::protocol_error, std::system_category(), stringf("%s off by %+lld for %s", oSAFESTRN(_DateType), _ExpectedValue-_CalculatedValue, oSAFESTRN(_DateName)));
}

static void throw_failed_date(const char* _DateName, const char* _DateType, const date& _ExpectedValue, const date& _CalculatedValue)
{
	throw std::system_error(std::errc::protocol_error, std::system_category(), std::string("Conversion back to date failed for ") + oSAFESTRN(_DateName));
}

#define oTESTDATE(_Test, _LLExpected, _LLCalculated) do \
	{	if (_Test._LLExpected != _LLCalculated) \
		throw_failed_date(_Test.ExpectedTimeString, #_LLExpected, _Test._LLExpected, _LLCalculated); \
	} while(false)

template<typename DateT1, typename DateT2>
static void expected_fail(const DateT1& _Destination, const DateT2& _Source)
{
	bool ExceptionThrown = false;
	oTRACE("Expecting a domain_exception failure as part of test...");
	try { DateT1 d = date_cast<DateT1>(_Source); }
	catch (...) { ExceptionThrown = true; }
	if (!ExceptionThrown)
		throw std::system_error(std::errc::protocol_error, std::system_category(), typeid(DateT1).name() + std::string(" = date_cast(") + typeid(DateT2).name() + ") succeeded for invalid date %s");
}

#define oTESTDATENOCONVERT(_Test, src, pdst) do { expected_fail(*pdst, src); } while(false)

#define oTESTDATEMILLI(_Test, _IExpected, _ICalculated) do \
{	if (_IExpected != _ICalculated) \
		throw std::system_error(std::errc::protocol_error, std::system_category(), stringf("Fractional sec should have been %dms for %s", _IExpected, oSAFESTRN(_Test.ExpectedTimeString))); \
} while(false)

static void test_date_julian()
{
	for (auto i = 0; i < countof(sNTPTests); i++)
	{
		const oNTPDATE_TEST& T = sNTPTests[i];
		long long JDN = julian_day_number(T.Date);
		oTESTDATE(T, JulianDay, JDN);
		long long MJD = static_cast<long long>(floor(modified_julian_date(T.Date)));
		oTESTDATE(T, ModifiedJulianDay, MJD);
	}
}

static void test_date_unix_time()
{
	for (auto i = 0; i < countof(sNTPTests); i++)
	{
		const oNTPDATE_TEST& T = sNTPTests[i];
		if (T.Date >= unix_time_epoch_start && T.Date <= unix_time_epoch_end_signed32)
		{
			ntp_date NTPDate = date_cast<ntp_date>(T.UnixTime);
			int era = get_ntp_era(NTPDate);
			oTESTDATE(T, NTPEra, era);

			unsigned int timestamp = get_ntp_timestamp(NTPDate);
			oTESTDATE(T, NTPTimestamp, timestamp);

			if (T.NTPEra == 0)
			{
				ntp_timestamp NTPTimestamp = date_cast<ntp_timestamp>(T.UnixTime);
				oTESTDATE(T, NTPTimestamp, get_ntp_timestamp(NTPTimestamp));
			}

			file_time_t ft = date_cast<file_time_t>(T.UnixTime);
			oTESTDATE(T, FileTime, ft);

			date Date = date_cast<date>(T.Date);
			oTESTDATE(T, Date, Date);
		}
	}
}

static void test_date_file_time()
{
	for (auto i = 0; i < countof(sNTPTests); i++)
	{
		const oNTPDATE_TEST& T = sNTPTests[i];
		if (T.Date >= file_time_epoch_start)
		{
			ntp_date NTPDate = date_cast<ntp_date>(T.FileTime);
			int era = get_ntp_era(NTPDate);
			oTESTDATE(T, NTPEra, era);

			unsigned int timestamp = get_ntp_timestamp(NTPDate);
			oTESTDATE(T, NTPTimestamp, timestamp);

			if (T.NTPEra == 0)
			{
				ntp_timestamp NTPTimestamp = date_cast<ntp_timestamp>(T.FileTime);
				oTESTDATE(T, NTPTimestamp, get_ntp_timestamp(NTPTimestamp));
			}

			if (T.Date >= unix_time_epoch_start && T.Date <= unix_time_epoch_end_signed32)
			{
				time_t t = date_cast<time_t>(T.FileTime);
				oTESTDATE(T, UnixTime, t);
			}

			date Date = date_cast<date>(T.Date);
			oTESTDATE(T, Date, Date);
		}
	}
}

static void test_ntp_timestamp(const oNTPDATE_TEST& _Test, int _ExpectedMilliseconds, const ntp_timestamp& _NTPTimestamp)
{
	unsigned int timestamp = get_ntp_timestamp(_NTPTimestamp);
	oTESTDATE(_Test, NTPTimestamp, timestamp);
	
	int MS = static_cast<int>(round(get_ntp_fractional_seconds(_NTPTimestamp) * 1000));
	oTESTDATEMILLI(_Test, _ExpectedMilliseconds, MS);
}

static void test_date_ntp(unit_test::services& services)
{
	for (auto i = 0; i < countof(sNTPTests); i++)
	{
		const oNTPDATE_TEST& T = sNTPTests[i];
		date d = T.Date;
		d.millisecond = services.rand() % 1000;
		
		ntp_date NTPDateNoMS = date_cast<ntp_date>(T.Date);
		ntp_date NTPDate = date_cast<ntp_date>(d);

		int era = get_ntp_era(NTPDateNoMS);
		oTESTDATE(T, NTPEra, era);
		 
		unsigned int timestamp = get_ntp_timestamp(NTPDateNoMS);
		oTESTDATE(T, NTPTimestamp, timestamp);
		
		int MS = static_cast<int>(round(get_ntp_fractional_seconds(NTPDate) * 1000));
		oTESTDATEMILLI(T, d.millisecond, MS);

		long long NTPDatePart = get_ntp_date(NTPDateNoMS);
		oTESTDATE(T, NTPDate, NTPDatePart);

		ntp_timestamp NTPTimestamp = 0;
		if (T.NTPEra == 0)
		{
			NTPTimestamp = date_cast<ntp_timestamp>(d);
			test_ntp_timestamp(T, d.millisecond, NTPTimestamp);
		}
		else 
			oTESTDATENOCONVERT(T, d, &NTPTimestamp);

		NTPTimestamp = 0;
		if (T.NTPEra == 0)
		{
			NTPTimestamp = date_cast<ntp_timestamp>(NTPDate);
			test_ntp_timestamp(T, d.millisecond, NTPTimestamp);
		}
		else 
			oTESTDATENOCONVERT(T, d, &NTPTimestamp);

		date TestDate = date_cast<date>(NTPDateNoMS);
		oTESTDATE(T, Date, TestDate);

		if (T.NTPEra == 0)
		{
			memset(&TestDate, 0, sizeof(TestDate));
			NTPTimestamp = date_cast<ntp_timestamp>(T.Date);
			TestDate = date_cast<date>(NTPTimestamp);
			oTESTDATE(T, Date, TestDate);

			file_time_t ft = date_cast<file_time_t>(NTPTimestamp);
			oTESTDATE(T, FileTime, ft);
		}

		time_t t = 0;
		if (T.Date >= unix_time_epoch_start && T.Date <= unix_time_epoch_end_signed32)
		{
			t = date_cast<time_t>(T.Date);
			oTESTDATE(T, UnixTime, t);
		}
		else
			oTESTDATENOCONVERT(T, T.Date, &t);

		file_time_t ft;
		if (T.Date >= file_time_epoch_start)
		{
			ft = date_cast<file_time_t>(T.Date);
			oTESTDATE(T, FileTime, (long long)ft);
			ft = date_cast<file_time_t>(NTPDateNoMS);
			oTESTDATE(T, FileTime, (long long)ft);
		}
		else
		{
			oTESTDATENOCONVERT(T, T.Date, &ft);
			oTESTDATENOCONVERT(T, NTPDateNoMS, &ft);
		}

		char StrDate[128];
		strftime(StrDate, T.TimeFormat, NTPDateNoMS);
		
		if (strcmp(StrDate, T.ExpectedTimeString))
			throw std::system_error(std::errc::protocol_error, std::system_category(), std::string("mal-formatted time for ") + oSAFESTRN(T.ExpectedTimeString));
	}
}

oTEST(oBase_date)
{
	test_date_julian();
	test_date_ntp(srv);
	test_date_unix_time();
	test_date_file_time();
}
