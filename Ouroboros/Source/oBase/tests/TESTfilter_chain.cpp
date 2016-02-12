// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/filter_chain.h>
#include <oCore/countof.h>
#include <oString/string.h>

using namespace ouro;

oTEST(oBase_filter_chain)
{
	filter_chain::filter_t filters[] =
	{
		{ ".*", filter_chain::inclusion::include1 },
		{ "a+.*", filter_chain::inclusion::exclude1 },
		{ "(ab)+.*", filter_chain::inclusion::include1 },
		{ "aabc", filter_chain::inclusion::include1 },
	};

	const char* symbols[] =
	{
		"test to succeed",
		"a test to fail",
		"aaa test to fail",
		"abab test to succeed",
		"aabc",
	};

	bool expected[] =
	{
		true,
		false,
		false,
		true,
		true,
	};

	filter_chain FilterChain(filters);
	for (auto i = 0; i < countof(symbols); i++)
		oCHECK(FilterChain.passes(symbols[i]) == expected[i], "Failed filter on %d%s symbol", i, ordinal(i));
}
