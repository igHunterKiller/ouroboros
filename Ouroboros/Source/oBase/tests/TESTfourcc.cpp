// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/fourcc.h>
#include <oCore/stringize.h>

using namespace ouro;

oTEST(oBase_fourcc)
{
	fourcc_t fcc('TEST');

	char str[16];

	oCHECK(to_string(str, fcc), "to_string on fourcc failed 1");
	oCHECK(!strcmp("TEST", str), "to_string on fourcc failed 2");

	const char* fccStr = "RGBA";
	oCHECK(from_string(&fcc, fccStr), "from_string on fourcc failed 1");
	oCHECK(fourcc_t('RGBA') == fcc, "from_string on fourcc failed 2");
}
