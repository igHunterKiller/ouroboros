// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// encapsulation of a fourcc code: http://en.wikipedia.org/wiki/code_

#pragma once
#include <cstdint>

// Prefer using the class fourcc_t, but here's a macro for those cases where 
// static composition is required.
#define oFOURCC(a, b, c, d) ((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((c)&0xff)<<8)|((d)&0xff))

// An endian-swapped version
#define oFOURCC_REV(x) oFOURCC((x),((x)>>8),((x)>>16),((x)>>24))

// "Readable" fourcc_t like 'fcc1' are not the right order on little-endian 
// machines, so either use '1ccf' or the oFCC macro
#define oFCC(x) (ouro::fourcc_t::platform_little_endian() ? oFOURCC_REV(x) : (x))

namespace ouro {

class fourcc_t
{
public:
	inline fourcc_t()                                     {}
	inline fourcc_t(int32_t fcc)  : code_((uint32_t&)fcc) {}
	inline fourcc_t(uint32_t fcc) : code_(fcc)            {}
	inline fourcc_t(const char* fcc_string)               { code_ = oFOURCC_REV(*(uint32_t*)fcc_string); }
	inline fourcc_t(char a, char b, char c, char d)       { code_ = oFOURCC(a, b, c, d); }

	inline operator int32_t () const { return *(int32_t*)&code_; }
	inline operator uint32_t() const { return code_; }

	inline bool operator< (const fourcc_t& that) const { return code_ < that.code_; }
	inline bool operator> (const fourcc_t& that) const { return that < *this; }
	inline bool operator>=(const fourcc_t& that) const { return !(*this < that); }
	inline bool operator<=(const fourcc_t& that) const { return !(*this > that); }
	inline bool operator==(const fourcc_t& that) const { return code_ == that.code_; }
	inline bool operator!=(const fourcc_t& that) const { return !(*this == that); }
	
	inline bool operator< (uint32_t that) const { return code_ < that; }
	inline bool operator> (uint32_t that) const { return code_ > that; }
	inline bool operator>=(uint32_t that) const { return !(*this < that); }
	inline bool operator<=(uint32_t that) const { return !(*this > that); }
	inline bool operator==(uint32_t that) const { return code_ == that; }
	inline bool operator!=(uint32_t that) const { return !(*this == that); }

	inline bool operator< (int32_t that) const { return code_ < (uint32_t&)that; }
	inline bool operator> (int32_t that) const { return code_ > (uint32_t&)that; }
	inline bool operator>=(int32_t that) const { return !(*this < that); }
	inline bool operator<=(int32_t that) const { return !(*this > that); }
	inline bool operator==(int32_t that) const { return code_ == (uint32_t&)that; }
	inline bool operator!=(int32_t that) const { return !(*this == that); }

	// helper exposed for oFCC
	static const bool platform_little_endian() { uint16_t sig = 1; return *(const uint8_t*)&sig == 1; }

private:
	uint32_t code_;
};

}
