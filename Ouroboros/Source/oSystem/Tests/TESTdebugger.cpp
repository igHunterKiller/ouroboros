// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/countof.h>
#include <oSystem/debugger.h>

using namespace ouro;

oTEST(oSystem_debugger)
{
	static const char* sExpectedStack[] = 
	{
		"ouro::unit_test::test_oSystem_debugger",
		"ouro::unit_test::run",
		"ouro::unit_test::run_tests",
		"main",
	};

	debugger::symbol_t addresses[32];
	size_t nAddresses = debugger::callstack(addresses, 0);

	for (size_t i = 0; i < countof(sExpectedStack); i++)
	{
		debugger::symbol_info sym = debugger::translate(addresses[i]);
		//printf("%u: %s\n", i, sym.Name);
		oCHECK(!strcmp(sym.name, sExpectedStack[i]), "Mismatch on stack trace at level %u", i);
	}
}
