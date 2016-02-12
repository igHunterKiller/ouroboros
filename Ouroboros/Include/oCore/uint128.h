// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// 128-bit unsigned integer type

#pragma once
#include <cstdint>

namespace ouro {

class uint128
{
public:
	uint128() {}
	uint128(const uint64_t& x) : hi_(0), lo_(x) {}
	uint128(const uint64_t& hi_, const uint64_t& lo_) : hi_(hi_), lo_(lo_) {}
	uint128(const uint128& that) { operator=(that); }

	const uint128& operator=(const uint64_t& that) { hi_ = 0; lo_ = that; return *this; }
	const uint128& operator=(const uint128& that) { hi_ = that.hi_; lo_ = that.lo_; return *this; }

	operator uint8_t() const { return (uint8_t)lo_; }
	operator uint16_t() const { return (uint16_t)lo_; }
	operator uint32_t() const { return (uint32_t)lo_; }
	operator uint64_t() const { return lo_; }

  uint64_t lo() const { return lo_; }
  void lo(uint64_t x) { lo_ = x; }
  
  uint64_t hi() const { return hi_; }
  void hi(uint64_t x) { hi_ = x; }

	uint128 operator~() const { uint128 x(*this); x.hi_ = ~x.hi_; x.lo_ = ~x.lo_; return *this; }
	uint128 operator-() const { return ~(*this) + uint128(1); }

	bool operator< (const uint128& that) const { return (hi_ == that.hi_) ? lo_ < that.lo_ : hi_ < that.hi_; }
	bool operator> (const uint128& that) const { return that < *this; }
	bool operator>=(const uint128& that) const { return !(*this < that); }
	bool operator<=(const uint128& that) const { return !(*this > that); }
	bool operator==(const uint128& that) const { return hi_ == that.hi_ && lo_ == that.lo_; }
	bool operator!=(const uint128& that) const { return !(*this == that); }

	uint128& operator++() { if (0 == ++lo_) hi_++; return *this; }
	uint128& operator--() { if (0 == lo_--) hi_--; return *this; }
	
	friend uint128 operator++(uint128& this_, int) { uint128 x(this_); ++this_; return x; }
	friend uint128 operator--(uint128& this_, int) { uint128 x(this_); --this_; return x; }
	
	uint128& operator|=(const uint128& that) { hi_ |= that.hi_; lo_ |= that.lo_; return *this; }
	uint128& operator&=(const uint128& that) { hi_ &= that.hi_; lo_ &= that.lo_; return *this; }
	uint128& operator^=(const uint128& that) { hi_ ^= that.hi_; lo_ ^= that.lo_; return *this; }

	friend uint128 operator|(const uint128& this_, const uint128& that) { uint128 x(this_); x |= that; return x; }
	friend uint128 operator&(const uint128& this_, const uint128& that) { uint128 x(this_); x &= that; return x; }
	friend uint128 operator^(const uint128& this_, const uint128& that) { uint128 x(this_); x ^= that; return x; }

	uint128& operator+=(const uint128& that) { hi_ += that.hi_; uint64_t tmp = lo_; lo_ += that.lo_; if (lo_ < tmp) hi_++; }
	uint128& operator-=(const uint128& that) { operator+=(-that); }

	friend uint128 operator+(const uint128& this_, const uint128& that) { uint128 x(this_); x += that; return x; }
	friend uint128 operator-(const uint128& this_, const uint128& that) { uint128 x(this_); x -= that; return x; }

	uint128& operator<<=(const uint32_t& x)
	{
		if (x >= 64) { hi_ = lo_; lo_ = 0; return operator<<=(x-64); }
		hi_ = (hi_ << x) | (lo_ >> (64-x)); lo_ = (lo_ << x);
		return *this;
	}

	uint128& operator>>=(const uint32_t& x)
	{
		if (x >= 64) { lo_ = hi_; hi_ = 0; return operator>>=(x-64); }
		lo_ = (hi_ << (64-x)) | (lo_ >> x); hi_ = (hi_ >> x);
		return *this;
	}

	friend uint128 operator<<(const uint128& this_, const uint128& that) { uint128 x(this_); x <<= that; return x; }
	friend uint128 operator>>(const uint128& this_, const uint128& that) { uint128 x(this_); x >>= that; return x; }

private:
	uint64_t hi_;
	uint64_t lo_;
};

}

// put in global namespace, consistent with other stdints
typedef ouro::uint128 uint128_t;
