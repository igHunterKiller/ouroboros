// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oString/path.h>
#include <oCore/countof.h>

using namespace ouro;

template<typename charT>
struct CASE
{
	const charT* CtorArgument;
	const charT* Iteration;
	const charT* AsStr;
	const charT* AsGenericStr;
	const charT* RootPath;
	const charT* RootName;
	const charT* RootDir;
	const charT* RelPath;
	const charT* ParentPath;
	const charT* Filename;
};

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3239.html#Path-decomposition-table
static const CASE<char> kCases[] = 
{
	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, },
	{ ".", ".", ".", ".", nullptr, nullptr, nullptr, ".", nullptr, ".", },
	{ "..", "..", "..", "..", nullptr, nullptr, nullptr, "..", nullptr, "..", },
	{ "foo", "foo", "foo", "foo", nullptr, nullptr, nullptr, "foo", nullptr, "foo", },
	{ "/", "/", "/", "/", "/", nullptr, "/", nullptr, nullptr, "/", },
	{ "/foo", "/,foo", "/foo", "/foo", "/", nullptr, "/", "foo", "/", "foo" },
	{ "foo/", "foo,.", "foo/", "foo/", nullptr, nullptr, nullptr, "foo/", "foo", "." },
	{ "/foo/", "/,foo,.", "/foo/", "/foo/", "/", nullptr, "/", "foo/", "/foo", "." },
	{ "foo/bar", "foo,bar", "foo/bar", "foo/bar", nullptr, nullptr, nullptr, "foo/bar", "foo", "bar" },
	{ "/foo/bar", "/,foo,bar", "/foo/bar", "/foo/bar", "/", nullptr, "/", "foo/bar", "/foo", "bar" },
	{ "//net", "//net", "//net", "//net", "//net", "//net", nullptr, nullptr, nullptr, "//net" },
	{ "//net/foo", "//net,/,foo", "//net/foo", "//net/foo", "//net/", "//net", "/", "foo", "//net/", "foo" },
	{ "///foo///", "/,foo,.", "///foo///", "///foo///", "/", nullptr, "/", "foo///", "///foo", "." },
	{ "///foo///bar", "/,foo,bar", "///foo///bar", "///foo///bar", "/", nullptr, "/", "foo///bar", "///foo", "bar" },
	{ "/.", "/,.", "/.", "/.", "/", nullptr, "/", ".", "/", "." },
	{ "./", ".,.", "./", "./", nullptr, nullptr, nullptr, "./", ".", "." },
	{ "/..", "/,..", "/..", "/..", "/", nullptr, "/", "..", "/", ".." },
	{ "../", "..,.", "../", "../", nullptr, nullptr, nullptr, "../", "..", "." },
	{ "foo/.", "foo,.", "foo/.", "foo/.", nullptr, nullptr, nullptr, "foo/.", "foo", "." },
	{ "foo/..", "foo,..", "foo/..", "foo/..", nullptr, nullptr, nullptr, "foo/..", "foo", ".." },
	{ "foo/./", "foo,.,.", "foo/./", "foo/./", nullptr, nullptr, nullptr, "foo/./", "foo/.", "." },
	{ "foo/./bar", "foo,.,bar", "foo/./bar", "foo/./bar", nullptr, nullptr, nullptr, "foo/./bar", "foo/.", "bar" },
	{ "foo/..", "foo,..", "foo/..", "foo/..", nullptr, nullptr, nullptr, "foo/..", "foo", ".." },
	{ "foo/../", "foo,..,.", "foo/../", "foo/../", nullptr, nullptr, nullptr, "foo/../", "foo/..", "." },
	{ "foo/../bar", "foo,..,bar", "foo/../bar", "foo/../bar", nullptr, nullptr, nullptr, "foo/../bar", "foo/..", "bar" },
	#if 0//defined(_WIN32) || defined(_WIN64)
		{ "c:", "c:", "c:", "c:", "c:", "c:", nullptr, nullptr, nullptr, "c:" },
		{ "c:/", "c:,/", "c:/", "c:/", "c:/", "c:", "/", nullptr, "c:", "/" },
		{ "c:foo", "c:,foo", "c:foo", "c:foo", "c:", "c:", nullptr, "foo", "c:", "foo" },
		{ "c:/foo", "c:,/,foo", "c:/foo", "c:/foo", "c:/", "c:", "/", "foo", "c:/", "foo" },
		{ "c:foo/", "c:,foo,.", "c:foo/", "c:foo/", "c:", "c:", nullptr, "foo/", "c:foo", "." },
		{ "c:/foo/", "c:,/,foo,.", "c:/foo/", "c:/foo/", "c:/", "c:", "/", "foo/", "c:/foo", "." },
		{ "c:/foo/bar", "c:,/,foo,bar", "c:/foo/bar", "c:/foo/bar", "c:/", "c:", "/", "foo/bar", "c:/foo", "bar" },
		{ "prn:", "prn:", "prn:", "prn:", "prn:", "prn:", nullptr, nullptr, nullptr, "prn:" },
		{ "c:\\", "c:,/", "c:\\", "c:/", "c:\\", "c:", "\\", nullptr, "c:", "\\" },
		{ "c:\\foo", "c:,/,foo", "c:\\foo", "c:/foo", "c:\\", "c:", "\\", "foo", "c:\\", "foo" },
		{ "c:foo\\", "c:,foo,.", "c:foo\\", "c:foo/", "c:", "c:", nullptr, "foo\\", "c:foo", "." },
		{ "c:\\foo\\", "c:,/,foo,.", "c:\\foo\\", "c:/foo/", "c:\\", "c:", "\\", "foo\\", "c:\\foo", "." },
		{ "c:\\foo/", "c:,/,foo,.", "c:\\foo/", "c:/foo/", "c:\\", "c:", "\\", "foo/", "c:\\foo", "." },
		{ "c:/foo\\bar", "c:,/,foo,bar", "c:/foo\\bar", "c:/foo/bar", "c:/", "c:", "/", "foo\\bar", "c:/foo", "bar" },
	#else
		{ "c:", "c:", "c:", "c:", nullptr, nullptr, nullptr, "c:", nullptr, "c:" },
		{ "c:/", "c:,.", "c:/", "c:/", nullptr, nullptr, nullptr, "c:/", "c:", "." },
		{ "c:foo", "c:foo", "c:foo", "c:foo", nullptr, nullptr, nullptr, "c:foo", nullptr, "c:foo" },
		{ "c:/foo", "c:,foo", "c:/foo", "c:/foo", nullptr, nullptr, nullptr, "c:/foo", "c:", "foo" },
		{ "c:foo/", "c:foo,.", "c:foo/", "c:foo/", nullptr, nullptr, nullptr, "c:foo/", "c:foo", "." },
		{ "c:/foo/", "c:,foo,.", "c:/foo/", "c:/foo/", nullptr, nullptr, nullptr, "c:/foo/", "c:/foo", "." },
		{ "c:/foo/bar", "c:,foo,bar", "c:/foo/bar", "c:/foo/bar", nullptr, nullptr, nullptr, "c:/foo/bar", "c:/foo", "bar" },
		{ "prn:", "prn:", "prn:", "prn:", nullptr, nullptr, nullptr, "prn:", nullptr, "prn:" },
		{ "c:\\", "c:\\", "c:\\", "c:\\", nullptr, nullptr, nullptr, "c:\\", nullptr, "c:\\" },
		{ "c:\\foo", "c:\\foo", "c:\\foo", "c:\\foo", nullptr, nullptr, nullptr, "c:\\foo", nullptr, "c:\\foo" },
		{ "c:foo\\", "c:foo\\", "c:foo\\", "c:foo\\", nullptr, nullptr, nullptr, "c:foo\\", nullptr, "c:foo\\" },
		{ "c:\\foo\\", "c:\\foo\\", "c:\\foo\\", "c:\\foo\\", nullptr, nullptr, nullptr, "c:\\foo\\", nullptr, "c:\\foo\\" },
		{ "c:\\foo/", "c:\\foo,.", "c:\\foo/", "c:\\foo/", nullptr, nullptr, nullptr, "c:\\foo/", "c:\\foo", "." },
		{ "c:/foo\\bar", "c:,foo\bar", "c:/foo\\bar", "c:/foo\\bar", nullptr, nullptr, nullptr, "c:/foo\\bar", "c:", "foo\\bar" },
	#endif
};

template<typename charT>
struct boost_and_std_proposal_compliant : path_traits<charT, true, false> {};

typedef basic_path_t<char, boost_and_std_proposal_compliant<char>> test_path;

static void TESTpath_standard_cases(unit_test::services& services)
{
	int i = 0;
	for (const auto& c : kCases)
	{
		try
		{
			test_path P(c.CtorArgument);

			#define TEST_CASE(_API, _Field) do \
			{	auto test__ = P._API(); \
				oCHECK(c._Field || !P.has_##_API(), "\"%s\"." "has_" #_API "() returned true when it should be false (thinks its \"%s\")", c.CtorArgument ? c.CtorArgument : "(empty)", test__.c_str()); \
				oCHECK(!c._Field || !strcmp(test__, c._Field), "\"%s\"." #_API "(): expected \"%s\", got \"%s\"" \
						, c.CtorArgument ? c.CtorArgument : "(empty)", c._Field ? c._Field : "empty", test__.empty() ? "(empty)" : test__.c_str()); \
			} while (false)

			path_t::string_type test = P.string();
			oCHECK(c.AsStr || test.empty(), "string() should have an empty value instead of \"%s\"", test.c_str());

			oCHECK(!c.AsStr || !strcmp(test, c.AsStr), "\"%s\".string(): expected \"%s\", got \"%s\""
					, test.c_str(), c.AsStr ? c.AsStr : "empty"
					, test.empty() ? "(empty)" : test.c_str());

			//TEST_CASE(generic_string, AsGenericStr);
			TEST_CASE(root_path, RootPath);
			TEST_CASE(root_name, RootName);
			TEST_CASE(root_directory, RootDir);
			TEST_CASE(relative_path, RelPath);
			TEST_CASE(parent_path, ParentPath);
			TEST_CASE(filename, Filename);
		}

		catch (std::exception& e)
		{
			throw std::logic_error(stringf("path test %d failed: %s", i, e.what()));
		}

		i++;
	}
}

template<typename charT>
struct CASE2
{
	const charT* CtorArgument;
	const charT* Base;
	const charT* Ext;
	const charT* Stem;
};

static const CASE2<char> kMoreCases[] = 
{
	{ nullptr, nullptr, nullptr, nullptr },
	{ "", nullptr, nullptr, nullptr },
	{ "c:/foo/bar/base.ext", "base", ".ext", "c:/foo/bar/base" },
	{ "c:/foo/bar/.ext", nullptr, ".ext", "c:/foo/bar/" },
	{ "c:/foo/bar/base", "base", nullptr, "c:/foo/bar/base" },
	{ "c:/foo/bar/base.not-ext.not-ext.ext", "base.not-ext.not-ext", ".ext", "c:/foo/bar/base.not-ext.not-ext" },
};

static void TESTpath_more_cases(unit_test::services& services)
{
	for (auto i = 0; i < countof(kMoreCases); i++)
	{
		try
		{
			const CASE2<char>& c = kMoreCases[i];
 			test_path P(c.CtorArgument);

			#define TEST_CASE2(_API, _Field) do \
			{ auto test = P._API(); \
				oCHECK(c._Field || test.empty(), #_API "() should have an empty value instead of \"%s\"", test.c_str()); \
				oCHECK(!c._Field || !strcmp(test, c._Field), "\"%s\"." #_API "(): expected \"%s\", got \"%s\"" \
						, test.c_str(), c._Field ? c._Field : "empty", test.empty() ? "(empty)" : test.c_str()); \
			} while (false)

			TEST_CASE2(basename, Base);
			TEST_CASE2(extension, Ext);
			TEST_CASE2(stem, Stem);
		}

		catch (std::exception& e)
		{
			throw std::logic_error(stringf("path test2 %d failed: %s", i, e.what()));
		}
	}
}

template<typename charT>
struct CLEAN_CASE
{
	const charT* CtorArgument;
	const charT* Clean;
};

static const CLEAN_CASE<char> kCleanCases[] = 
{
	{ "c:/my//path", "c:/my/path" },
	{ "//c/my/path", "//c/my/path" },
	{ "C:/AutoBuild/1.0.002.6398//../WebRoot/26417/oUnitTests.txt.stderr", "C:/AutoBuild/WebRoot/26417/oUnitTests.txt.stderr" },
};

static void TESTpath_clean(unit_test::services& services)
{
	for (auto i = 0; i < countof(kCleanCases); i++)
	{
		try
		{
			const CLEAN_CASE<char>& c = kCleanCases[i];
			path_t P(c.CtorArgument);
			oCHECK(!strcmp(P, c.Clean), "\"%s\".clean() expected \"%s\", got \"%s\"", c.CtorArgument, c.Clean, P.c_str());
		}

		catch (std::exception& e)
		{
			throw std::logic_error(stringf("path test2 %d failed: %s", i, e.what()));
		}
	}
}

oTEST(oString_path)
{
	TESTpath_standard_cases(srv);
	TESTpath_more_cases(srv);
	TESTpath_clean(srv);

	test_path P("c:/foo/bar/img.png");

	P.replace_extension(".jpg");
	oCHECK(!strcmp(P, "c:/foo/bar/img.jpg"), "replace_extension failed");

	P.replace_extension("bmp");
	oCHECK(!strcmp(P, "c:/foo/bar/img.bmp"), "replace_extension failed");

	P.replace_filename("file.txt");
	oCHECK(!strcmp(P, "c:/foo/bar/file.txt"), "replace_filename failed");

	P.remove_filename();
	oCHECK(!strcmp(P, "c:/foo/bar/"), "remove_leaf failed");

	P.remove_filename();
	oCHECK(!strcmp(P, "c:/foo/bar"), "remove_leaf failed");

	P = "/";
	P.remove_filename();
	oCHECK(P.empty(), "remove_leaf failed");
}
