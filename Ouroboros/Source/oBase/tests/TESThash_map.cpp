// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oBase/growable_hash_map.h>
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

oTEST(oBase_hash_map)
{
	typedef unsigned long long hash_t;

	typedef growable_hash_map<hash_t, std::string> hash_map_t;
	static const unsigned int NumElements = 10;

	std::vector<std::string> Strings;
	generate_strings(srv, Strings, NumElements);
	std::vector<hash_t> Hashes;
	Hashes.reserve(Strings.size());

	for (const auto& s : Strings)
		Hashes.push_back(fnv1a<hash_t>(s.c_str()));

	hash_map_t map2(NumElements);
	hash_map_t map1(std::move(map2));
	hash_map_t map = std::move(map1);

	for (size_t i = 0; i < Strings.size(); i++)
		oCHECK(map.add(Hashes[i], Strings[i]) || map.exists(Hashes[i]), "failed to add entry");

	oCHECK(map.size() == Strings.size(), "unexpected map size");
	size_t CountedEntries = 0;
	map.enumerate_const([&](const hash_t& _HashKey, const std::string& _Value)->bool
	{
		oCHECK(std::find(Hashes.begin(), Hashes.end(), _HashKey) != Hashes.end(), "traversing a hash that was not inserted");
		CountedEntries++;
		return true;
	});
	
	oCHECK(CountedEntries == Strings.size(), "not all entries were added");
	CountedEntries = 0;

	map.enumerate([&](const hash_t& _HashKey, std::string& _Value)->bool
	{
		oCHECK(std::find(Hashes.begin(), Hashes.end(), _HashKey) != Hashes.end(), "traversing a hash that was not inserted");
		CountedEntries++;
		return true;
	});
	
	oCHECK(CountedEntries == Strings.size(), "not all entries were added");
	CountedEntries = 0;

	map.resize(NumElements * 2);
	map.enumerate_const([&](const hash_t& _HashKey, const std::string& _Value)->bool
	{
		oCHECK(std::find(Hashes.begin(), Hashes.end(), _HashKey) != Hashes.end(), "traversing a hash that was not inserted");
		CountedEntries++;
		return true;
	});
	
	oCHECK(CountedEntries == Strings.size(), "resize failed");

	for (size_t i = 0; i < Strings.size(); i += 2)
		oCHECK(map.remove(Hashes[i]), "failed to remove an entry known to exist");

	oCHECK(map.size() == Strings.size() / 2, "unexpected map size");

	for (size_t i = 0; i < Strings.size(); i++)
	{
		if (i & 0x1)
			oCHECK(map.exists(Hashes[i]), "an entry that should be present was not found");
		else
			oCHECK(!map.exists(Hashes[i]), "found an entry that was just removed");
	}

	unsigned int ExpectedRemoval = 0;
	for (size_t i = 0; i < Strings.size(); i += 3)
		if (map.mark(Hashes[i], 0xdeadc0de))
			ExpectedRemoval++;

	unsigned int Removed = map.sweep(0xdeadc0de);
	oCHECK(ExpectedRemoval == Removed, "removed a different number of entries than marked");

	oCHECK(map.size() == (Strings.size() / 2) - Removed, "unexpected map size");

	map.clear();
	oCHECK(map.empty(), "unexpected map size");
}
