// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/filesystem.h>

using namespace ouro;

int main(int argc, const char* argv[])
{
	if (argc <= 1)
	{
		path_t exe(argv[0]);
		printf("%s <infile> [<outfile>]\n  If no outfile is specified, infile_ext.h is used.\n", exe.filename().c_str());
		return true;
	}

	path_t infile(argv[1]);
	path_t outfile;

	if (argc > 2)
		outfile = argv[2];
	else
	{
		outfile = infile;
		auto ext = infile.extension();
		char suffix[64];
		if (ext.empty())
			snprintf(suffix, ".h");
		else
			snprintf(suffix, "_%s.h", infile.extension().c_str() + 1);

		outfile.replace_extension_with_suffix(suffix);
	}

	auto Name = outfile.basename();

	try
	{
		auto b = filesystem::load(infile);

		std::string s;
		s.reserve(b.size() * 2 / 3);

		s = "const unsigned char ";
		s += Name.c_str();
		s += "[] = " oNEWLINE "{ // *** AUTO-GENERATED BUFFER, DO NOT EDIT *** (100b/row)";

		char buf[160];
		const unsigned char* p = (const unsigned char*)b;
		for (size_t i = 0; i < b.size(); i++)
		{
			int offset = 0;
			if ((i%100)==0)
				offset = snprintf(buf, oNEWLINE "\t");
			snprintf(buf+offset, sizeof(buf)-offset, "%u,", p[i]);
			s += buf;
		}

		snprintf(buf, oNEWLINE "}; // %s" oNEWLINE, Name.c_str());
		s += buf;

		filesystem::save(outfile, s.c_str(), s.length(), filesystem::save_option::binary_write);
	}

	catch (std::exception& e)
	{
		printf("unhandled exception: %s", e.what());
		return -1;
	}

	return 0;
}
