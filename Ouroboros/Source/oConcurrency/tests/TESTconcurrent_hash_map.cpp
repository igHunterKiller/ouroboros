// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oConcurrency/concurrent_hash_map.h>
#include <oMemory/fnv1a.h>
#include <string>
#include <vector>

using namespace ouro;

typedef concurrent_hash_map<uint64_t, uint32_t> hash_map_t;

static void generate_strings(unit_test::services& services, std::vector<std::string>& _Strings, size_t _NumToGenerate)
{
	_Strings.reserve(_NumToGenerate);

	for (size_t i = 0; i < _NumToGenerate; i++)
	{
		size_t StringLength = services.rand();
		std::string s;
		s.reserve(StringLength);

		for (size_t j = 0; j < StringLength; j++)
			s += (char)services.rand();

		_Strings.push_back(s);
	}
}

void TEST_chm_basics(unit_test::services& services)
{
	auto bytes = hash_map_t::calc_size(12);
	blob mem = default_allocator.scoped_allocate(bytes, "concurrent_hash_map test", hash_map_t::required_alignment);

	hash_map_t h(uint32_t(-1), mem, 12);
	oCHECK(h.capacity() == 32, "pow-of-two rounding failed");
	oCHECK(h.size() == 0, "should be empty");

	std::vector<std::string> Strings;
	generate_strings(services, Strings, 10);

	std::vector<hash_map_t::key_type> Keys;
	Keys.resize(Strings.size());
	for (size_t i = 0; i < Strings.size(); i++)
		Keys[i] = fnv1a<hash_map_t::key_type>(Strings[i].c_str());

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i += 3)
		h.set(Keys[i], i);

	oCHECK(h.size() == ((Strings.size() / 3) + 1), "set failed");

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i += 3)
		oCHECK(i == h.get(Keys[i]), "get failed");

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i++)
		h.set(Keys[i], i);

	oCHECK(h.size() == Strings.size(), "set2 failed");

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i += 3)
		h.nix(Keys[i]);

	oCHECK(h.size() == (Strings.size() - 4), "set2 failed");

	oCHECK(4 == h.reclaim_keys(), "reclaim failed");

	h.clear();
	oCHECK(h.empty(), "clear failed");
}

oTEST(oConcurrency_concurrent_hash_map)
{
	TEST_chm_basics(srv);
}
