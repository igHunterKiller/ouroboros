// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// http://en.wikipedia.org/wiki/Universally_Unique_Identifier
// {MSVSInstallDir}/Common7/Tools/guidgen.exe to generate these

#pragma once
#include <cstdint>

namespace ouro {

class guid_t
{
	typedef const uint64_t* ptr;

public:
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t  data4[8];

	bool operator< (const guid_t& that) const { return ptr(this)[0] < ptr(&that)[0] || (ptr(this)[0] == ptr(&that)[0] && ptr(this)[1] < ptr(&that)[1]); }
	bool operator> (const guid_t& that) const { return that < *this; }
	bool operator<=(const guid_t& that) const { return !(*this > that); }
	bool operator>=(const guid_t& that) const { return !(*this < that); }
	bool operator==(const guid_t& that) const { return ptr(this)[0] == ptr(&that)[0] && ptr(this)[1] == ptr(&that)[1]; }
	bool operator!=(const guid_t& that) const { return !(*this == that); }
};

static const guid_t null_guid = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

}

// printf-style format string for data1, data2, data3, data,4
#define oGUID_FMT "{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}"
#define oGUID_DATA_IN(x) (x).data1, (x).data2, (x).data3, (x).data4[0], (x).data4[1], (x).data4[2], (x).data4[3], (x).data4[4], (x).data4[5], (x).data4[6], (x).data4[7]
#define oGUID_DATA_OUT(x) &((x).data1), &((x).data2), &((x).data3), &((x).data4[0]), &((x).data4[1]), &((x).data4[2]), &((x).data4[3]), &((x).data4[4]), &((x).data4[5]), &((x).data4[6]), &((x).data4[7])
