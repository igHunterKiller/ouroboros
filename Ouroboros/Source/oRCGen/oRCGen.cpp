// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/opttok.h>
#include <oBase/vcs.h>
#include <oCore/assert.h>
#include <oCore/version.h>
#include <oSystem/filesystem.h>
#include <oSystem/system.h>

using namespace ouro;

static option sOptions[] = 
{
	{ 'v', "version", "maj.min", "Major/minor version to use" },
	{ 'a', "auto-revision", nullptr, "Obtain revision information from scc" },
	{ 'r', "revision", "scc-revision", "override auto-determination of source code control revision" },
	{ 'p', "ProductName", "product name", "Product name" },
	{ 'f', "FileName", "filename", "File name" },
	{ 'c', "Company", "name", "Company name" },
	{ 'C', "Copyright", "string", "Copyright" },
	{ 'd', "Description", "string", "Description" },
	{ 'o', "output", "path", "Output file path" },
	{ 'h', "help", 0, "Displays this message" },
};

struct oOPT
{
	const char* ProductName;
	const char* FileName;
	const char* Company;
	const char* Copyright;
	const char* Description;
	const char* Output;
	unsigned short Major;
	unsigned short Minor;
	unsigned int Revision;
	bool AutoRevision;
	bool ShowHelp;
};

oOPT ParseOptions(int argc, const char* argv[])
{
	oOPT o;
	memset(&o, 0, sizeof(oOPT));
	const char* value = nullptr;
	char ch = opttok(&value, argc, argv, sOptions);
	while (ch)
	{
		switch (ch)
		{
			case 'v':
			{
				sstring v(value);
				char* majend = strchr(v, '.');
				if (majend)
				{
					*majend++ = '\0';
					if (from_string(&o.Major, v) && from_string(&o.Minor, majend))
						break;
				}
				oThrow(std::errc::invalid_argument, "version number not well-formatted");
			}
			case 'a': o.AutoRevision = true; break;
			case 'r':
			{
				oCheck(from_string(&o.Revision, value), std::errc::invalid_argument, "unrecognized -r value: it must be an non-negative integer", value);
				break;
			}
			case 'p': o.ProductName = value; break;
			case 'f': o.FileName = value; break;
			case 'c': o.Company = value; break;
			case 'C': o.Copyright = value; break;
			case 'd': o.Description = value; break;
			case 'h': o.ShowHelp = true; break;
			case 'o': o.Output = value; break;
			case '?': oThrow(std::errc::invalid_argument, "unrecognized switch \"%s\"", value);
			case ':': oThrow(std::errc::invalid_argument, "missing parameter option for argument %d", (intptr_t)value);
		}

		ch = opttok(&value);
	}

	if (!o.Output)
		oThrow(std::errc::invalid_argument, "-o specifying an output file is required");

	return o;
}

int ShowHelp(const char* argv0)
{
	path_t path = argv0;
	char doc[1024];
	optdoc(doc, path.filename().c_str(), sOptions);
	printf("%s\n", doc);
	return 0;
}

int main(int argc, const char* argv[])
{
		throw std::exception("needs scc -> vcs migration");
#if 0
	try
	{
		oOPT opt = ParseOptions(argc, argv);

		if (opt.ShowHelp)
			return ShowHelp(argv[0]);

		unsigned int Revision = opt.Revision;
		if (opt.AutoRevision)
		{
			typedef std::function<int(const char* _Commandline
				, scc_get_line_fn get_line
				, void* user
				, unsigned int _TimeoutMS)> scc_spawn;

			auto scc = make_scc(scc_protocol::svn, 
			[](const char* cmdline, scc_get_line_fn get_line, void* user, uint32_t timeout_ms)->int
			{
				return system::spawn_for(cmdline, get_line, user, false, timeout_ms);

			}, nullptr);

			path_t DevPath = filesystem::dev_path();
			printf("scc");
			lstring RevStr;
			try { Revision = scc->revision(DevPath); }
			catch (std::exception& e) { RevStr = e.what(); }
			if (RevStr.empty())
				printf(" %s %s\n", Revision ? to_string(RevStr, Revision) : "?", DevPath.c_str());
			else
				printf(" ? %s %s\n", DevPath.c_str(), RevStr.c_str());
		}

		version_t v(opt.Major, opt.Minor);
		sstring VerStrMS, VerStrLX;
		xlstring s;
		int w = snprintf(s, 
			"// generated file - do not modify\n" \
			"#define oRC_SCC_REVISION %u\n" \
			"#define oRC_VERSION_VAL %u,%u,%u,%u\n" \
			"#define oRC_VERSION_STR_MS \"%s\"\n" \
			"#define oRC_VERSION_STR_LINUX \"%u.%u.%u\"\n" \
			, Revision
			, v.major, v.minor, v.build, v.revision
			, to_string(VerStrMS, v)
			, to_string(VerStrLX, v));

		if (opt.ProductName) sncatf(s, "#define oRC_PRODUCTNAME \"%s\"\n", opt.ProductName);
		if (opt.FileName) sncatf(s, "#define oRC_FILENAME \"%s\"\n", opt.FileName);
		if (opt.Company) sncatf(s, "#define oRC_COMPANY \"%s\"\n", opt.Company);
		if (opt.Copyright) sncatf(s, "#define oRC_COPYRIGHT \"%s\"\n", opt.Copyright);
		if (opt.Description) sncatf(s, "#define oRC_DESCRIPTION \"%s\"\n", opt.Description);

		filesystem::save(opt.Output, s, s.length(), filesystem::save_option::text_write);

	}

	catch (std::exception& e)
	{
		printf("%s\n", e.what());
		std::system_error* se = dynamic_cast<std::system_error*>(&e);
		return se ? se->code().value() : -1;
	}

	return 0;
#endif
}
