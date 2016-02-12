// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oString/ini.h>
#include <memory>

using namespace ouro;

static const char* sExpectedSectionNames[] = { "CD0", "CD1", "CD2", };
static const char* sExpectedArtists[] = { "Bob Dylan", "Bonnie Tyler", "Dolly Parton", };
static const char* sExpectedYears[] = { "1985", "1988", "1982", };
static const char* sExpectedCD2Values[] = { "Greatest Hits", "Dolly Parton", "USA", "RCA", "9.90", "1982" };

static const char* sTestINI = {
	"[CD0]\n" \
	"Title=Empire Burlesque\n" \
	"Artist=Bob Dylan\n" \
	"Country=USA\n" \
	"Company=Columbia\n" \
	"Price=10.90\n" \
	"Year=1985\n" \
	"\n" \
	"[CD1]\n" \
	"Title=Hide your heart\n" \
	"Artist=Bonnie Tyler\n" \
	"Country=UK\n" \
	"Company=CBS Records\n" \
	"Price=9.90\n" \
	"Year=1988\n" \
	"\n" \
	"[CD2]\n" \
	"Title=Greatest Hits\n" \
	"Artist=Dolly Parton\n" \
	"Country=USA\n" \
	"Company=RCA\n" \
	"Price=9.90\n" \
	"Year=1982\n"
};

static const char* sTestINI_RN = {
	"[CD0]\r\n" \
	"Title=Empire Burlesque\r\n" \
	"Artist=Bob Dylan\r\n" \
	"Country=USA\r\n" \
	"Company=Columbia\r\n" \
	"Price=10.90\r\n" \
	"Year=1985\r\n" \
	"\r\n" \
	"[CD1]\r\n" \
	"Title=Hide your heart\r\n" \
	"Artist=Bonnie Tyler\r\n" \
	"Country=UK\r\n" \
	"Company=CBS Records\r\n" \
	"Price=9.90\r\n" \
	"Year=1988\r\n" \
	"\r\n" \
	"[CD2]\r\n" \
	"Title=Greatest Hits\r\n" \
	"Artist=Dolly Parton\r\n" \
	"Country=USA\r\n" \
	"Company=RCA\r\n" \
	"Price=9.90\r\n" \
	"Year=1982\r\n" \
	"\r\n"
};

static void TESTini(unit_test::services& services, const char* _pData)
{
	static const char* ININame = "Test INI";
	std::shared_ptr<ini> INI = std::make_shared<ini>(ININame, _pData, 100);

	bool AtLeastOneExecuted = false;
	int i = 0;
	for (ini::section s = INI->first_section(); s; s = INI->next_section(s), i++)
	{
		oCHECK(!strcmp(INI->section_name(s), sExpectedSectionNames[i]), "%s: Name of %d%s section is incorrect", ININame, i, ordinal(i));
		oCHECK(!strcmp(INI->find_value(s, "Artist"), sExpectedArtists[i]), "%s: Artist in %d%s section did not match", ININame, i, ordinal(i));
		oCHECK(!strcmp(INI->find_value(s, "Year"), sExpectedYears[i]), "%s: Year in %d%s section did not match", ININame, i, ordinal(i));
		AtLeastOneExecuted = true;
	}

	oCHECK(AtLeastOneExecuted, "No sections were read");

	ini::section s = INI->find_section("CD2");
	oCHECK(s, "Section [CD2] could not be read");
	i = 0;
	for (ini::key k = INI->first_key(s); k; k = INI->next_key(k), i++)
		oCHECK(!strcmp(INI->value(k), sExpectedCD2Values[i]), "%s: Reading CD2 %d%s entry.", ININame, i, ordinal(i));
}

oTEST(oString_ini)
{
	TESTini(srv, sTestINI);
	TESTini(srv, sTestINI_RN);
}

