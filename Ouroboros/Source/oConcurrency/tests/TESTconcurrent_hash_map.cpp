// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oConcurrency/concurrent_hash_map.h>
#include <oMemory/fnv1a.h>
#include <string>
#include <vector>

using namespace ouro;

static void generate_strings(unit_test::services& services, std::vector<std::string>& _Strings, size_t _NumToGenerate)
{
	_Strings.reserve(_NumToGenerate);

	for (size_t i = 0; i < _NumToGenerate; i++)
	{
		size_t StringLength = services.rand();
		std::string s;
		s.reserve(StringLength);

		for (size_t i = 0; i < StringLength; i++)
			s += (char)services.rand();

		_Strings.push_back(s);
	}
}

void TEST_chm_basics(unit_test::services& services)
{
	auto bytes = concurrent_hash_map::calc_size(12);
	blob mem = default_allocator.scoped_allocate(bytes, "concurrent_hash_map test", concurrent_hash_map::required_alignment);

	concurrent_hash_map h(mem, 12);
	oCHECK(h.capacity() == 31, "pow-of-two rounding failed");
	oCHECK(h.size() == 0, "should be empty");

	std::vector<std::string> Strings;
	generate_strings(services, Strings, 10);

	std::vector<concurrent_hash_map::key_type> Keys;
	Keys.resize(Strings.size());
	for (size_t i = 0; i < Strings.size(); i++)
		Keys[i] = fnv1a<concurrent_hash_map::key_type>(Strings[i].c_str());

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i += 3)
		h.set(Keys[i], i);

	oCHECK(h.size() == ((Strings.size() / 3) + 1), "set failed");

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i += 3)
		oCHECK(i == h.get(Keys[i]), "get failed");

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i++)
		h.set(Keys[i], i);

	oCHECK(h.size() == Strings.size(), "set2 failed");

	for (unsigned int i = 0; i < static_cast<unsigned int>(Strings.size()); i += 3)
		h.remove(Keys[i]);

	oCHECK(h.size() == (Strings.size() - 4), "set2 failed");

	oCHECK(4 == h.reclaim(), "reclaim failed");

	h.clear();
	oCHECK(h.empty(), "clear failed");
}

oTEST(oConcurrency_concurrent_hash_map)
{
	TEST_chm_basics(srv);
}
