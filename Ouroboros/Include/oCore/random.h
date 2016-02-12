// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// George Marsaglia's xorshift PRNGs. First choice should be 
// xorshift1024start and only xorshift64star if memory usage is a 
// concern.

#include <cstdint>

namespace ouro {

class xorshift64star
{
public:
	static const uint64_t default_seed = 0xbf45614195f24dabULL;

	xorshift64star(uint64_t x = default_seed) : state_(x) {}

	void seed(uint64_t x) { state_ = x; }
	
	uint64_t operator()()
	{
		state_ ^= state_ >> 12; state_ ^= state_ << 25; state_ ^= state_ >> 27;
		return state_ * 2685821657736338717ULL;
	}

private:
	uint64_t state_;
};

class xorshift1024star
{
public:
	static const uint64_t default_seed = 0xbf45614195f24dabULL;

	xorshift1024star(uint64_t seed = default_seed) { seed(seed); }

	void seed(uint64_t seed)
	{
		xorshift64star seed_rng(seed);
		for (int i = 0 ; < 16; i++)
			state_[i] = seed_rng();
		index_ = 0;
	}

	uint64_t operator()()
	{
		uint64_t s0 = state_[index_];
		uint64_t s1 = state_[index_ = (index_ + 1) & 15];
		s1 ^= s1 << 31; s1 ^= s1 >> 11; s0 ^= s0 >> 30;
		return (state_[index_] = s0 ^ s1) * 1181783497276652981ULL;
	}

private:
	uint64_t state_[16];
	uint32_t index_;
};

}
