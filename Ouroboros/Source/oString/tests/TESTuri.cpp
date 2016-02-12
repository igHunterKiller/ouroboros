// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/countof.h>
#include <oCore/timer.h>
#include <oString/uri.h>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace ouro;

struct URI_PARTS_TEST
{
	const char* URI;
	const char* Scheme;
	const char* Authority;
	const char* Path;
	const char* Query;
	const char* Fragment;
};

static const URI_PARTS_TEST sTESTuri_parts[] =
{
	{ "file:///C:/trees/sys3/int/src/core/tests/TestString.cpp", "file", "", "/C:/trees/sys3/int/src/core/tests/TestString.cpp", "", "" },
	{ "http://msdn.microsoft.com/en-us/library/bb982727.aspx#regexgrammar", "http", "msdn.microsoft.com", "/en-us/library/bb982727.aspx", "", "regexgrammar" },
	{ "file://abyss/trees/sys3/int/src/core/all%20tests/TestString.cpp", "file", "abyss", "/trees/sys3/int/src/core/all tests/TestString.cpp", "", "" },
	{ "FiLe:///C:\\Test/../A file.TxT", "file", "", "/C:/A file.TxT", "", "" },
	{ "file:///c:/a%20file.txt", "file", "", "/c:/a file.txt", "", "" },
	{ "ftp:///c:/a%20file.txt", "ftp", "", "/c:/a file.txt", "", "" },
};

static void TESTuri_parts(unit_test::services& services)
{
	for (auto i = 0; i < countof(sTESTuri_parts); i++)
	{
		const auto& t = sTESTuri_parts[i];
		uri_t u(t.URI);

		auto s = u.scheme();
		auto a = u.authority();
		auto p = u.path();
		auto q = u.query();
		auto f = u.fragment();

		oCHECK0(!strcmp(s, t.Scheme));
		oCHECK0(!strcmp(a, t.Authority));
		oCHECK(!p.compare(t.Path), "fail(%d): path(%s) == %s", i, u.c_str(), t.Path);
		oCHECK0(!strcmp(q, t.Query));
		oCHECK0(!strcmp(f, t.Fragment));

		oCHECK0(u == u);
		if (i)
		{
			uri_t u2(sTESTuri_parts[i-1].URI);
			oCHECK(u != u2, "fail(%d): %s != %s", i, u.c_str(), u2.c_str());
		}
	}
}

struct URI_ABSOLUTE
{
	const char* String;
	bool IsAbsolute;
};

static const URI_ABSOLUTE sTESTuri_absolute[] =
{
	{ "http:", true },
	{ "file:", true },
	{ "http:path", true },
	{ "file://server/path", true },
	{ "file://server/path#fragment", false },
	{ "file://server/path?query", true },
	{ "file:../path/path2/file.ext", true },
	{ "#file:", false },
	{ "?file:", false },
	{ "./file:", false },
	{ "/file:", false },
	{ "file.txt", false },
	{ "file.txt#item", false },
	{ "file.txt?query=false", false },
	{ "//server/file.txt", false },
	{ "file:#file", false },
	{ "file:file#file", false },
	{ "http://server/file#file", false },
};

static void TESTuri_absolute(unit_test::services& services)
{
	for (auto i = 0; i < countof(sTESTuri_absolute); i++)
	{
		const auto& t = sTESTuri_absolute[i];
		uri_t u(t.String);
		oCHECK(u.absolute() == t.IsAbsolute, "fail(%d): %s is %sabsolute when %sexpected to be."
			, i
			, t.String
			, t.IsAbsolute ? "not " : ""
			, t.IsAbsolute ? "" : "not ");
	}
}

struct URI_SAME_DOCUMENT
{
	const char* String1;
	const char* String2;
	bool IsSame;
};

static const URI_SAME_DOCUMENT sTESTuri_same_document[] = 
{
	{ "", "file:///path/path2/file.txt", true },
	{ "#some_item", "file:///path/path2/file.txt", true },
	{ "file.txt", "file:///path/path2/file.txt", true },
	{ "file.txt?modified", "file:///path/path2/file.txt?modified", true },
	{ "file.txt#some_item", "file:///path/path2/file.txt", true },
	{ "../path2/file.txt#some_item", "file:///path/path2/file.txt", true },
	{ "/path/path2/file.txt#some_item", "file:///path/path2/file.txt", true },
	{ "file:path/path2/file.txt#some_item", "file:///path/path2/file.txt", true },
	{ "file:///path/path2/file.txt#some_item", "file:///path/path2/file.txt", true },

	{ "a", "file:///path/path2/file.txt", false },
	{ "a#some_item", "file:///path/path2/file.txt", false },
	{ "a/file.txt", "file:///path/path2/file.txt", false },
	{ "file.txt?notmodified", "file:///path/path2/file.txt?modified", false },
	{ "../path/file.txt#some_item", "file:///path/path2/file.txt", false },
	{ "/path/path/file.txt#some_item", "file:///path/path2/file.txt", false },
	{ "file:path/path/file.txt#some_item", "file:///path/path2/file.txt", false },
	{ "file:///path/path/file.txt#some_item", "file:///path/path2/file.txt", false },
};

void TESTuri_same_document(unit_test::services& services)
{
	for (auto i = 0; i < countof(sTESTuri_same_document); i++)
	{
		const auto& t = sTESTuri_same_document[i];
		uri_t a(t.String1), b(t.String2);
		oCHECK(a.is_same_document(b), "fail(%d): %s is_same_document %s", i, t.String1, t.String2);
	}
}

struct URI_RESOLVE
{
	const char* Ref;
	const char* Resolved;
};

static const URI_RESOLVE sTESTuri_make_absolute[] =
{
	// http://tools.ietf.org/html/rfc3986#section-5.4.1
	{ "g:h"	, "g:h" },
	{ "g", "http://a/b/c/g" },
	{ "./g", "http://a/b/c/g" },
	{ "g/", "http://a/b/c/g/" },
	{ "/g", "http://a/g" },
	{ "//g", "http://g" },
	{ "?y", "http://a/b/c/d;p?y" },
	{ "g?y", "http://a/b/c/g?y" },
	{ "#s", "http://a/b/c/d;p?q#s" },
	{ "g#s", "http://a/b/c/g#s" },
	{ "g?y#s", "http://a/b/c/g?y#s" },
	{ ";x", "http://a/b/c/;x" },
	{ "g;x", "http://a/b/c/g;x" },
	{ "g;x?y#s", "http://a/b/c/g;x?y#s" },
	{ "", "http://a/b/c/d;p?q" },
	{ ".", "http://a/b/c/" },
	{ "./", "http://a/b/c/" },
	{ "..", "http://a/b/" },
	{ "../", "http://a/b/" },
	{ "../g", "http://a/b/g" },
	{ "../..", "http://a/" },
	{ "../../", "http://a/" },
	{ "../../g", "http://a/g" },

	// http://tools.ietf.org/html/rfc3986#section-5.4.2
	{ "../../../g", "http://a/g" },
	{ "../../../../g", "http://a/g" },
	{ "/./g", "http://a/g" },
	{ "/../g", "http://a/g" },
	{ "g.", "http://a/b/c/g." },
	{ ".g", "http://a/b/c/.g" },
	//	{ "g..", "http://a/b/c/g.." }, // FIXME
	{ "..g", "http://a/b/c/..g" },
	{ "./../g", "http://a/b/g" },
	{ "./g/.", "http://a/b/c/g/" },
	{ "g/./h", "http://a/b/c/g/h" },
	{ "g/../h", "http://a/b/c/h" },
	{ "g;x=1/./y", "http://a/b/c/g;x=1/y" },
	{ "g;x=1/../y", "http://a/b/c/y" },
	{ "g?y/./x", "http://a/b/c/g?y/./x" },
	{ "g?y/../x", "http://a/b/c/g?y/../x" },
	{ "g#s/./x", "http://a/b/c/g#s/./x" },
	{ "g#s/../x", "http://a/b/c/g#s/../x" },
};
static_assert((countof(sTESTuri_make_absolute) % 2) == 0, "sTESTuri_make_absolute must be even so it can be divided into two threads");

static const URI_RESOLVE sTESTuri_make_absolute2[] = 
{
	{ "/test.xml", "file://DATA/test.xml" },
	{ "#Node", "file://DATA/Test/Scenes/TestTextureSet.xml#Node" },
	{ "../Textures/Hatch1.png", "file://DATA/Test/Textures/Hatch1.png" },
	{ "test.xml", "file://DATA/Test/Scenes/test.xml" },
	{ "file://DATA2/test.xml", "file://DATA2/test.xml" },
	{ "http://DATA/test.xml", "http://DATA/test.xml" },
	{ "http://DATA2/test.xml", "http://DATA2/test.xml" },
};

static const URI_RESOLVE sTESTuri_make_absolute3[] = 
{
	// http://tools.ietf.org/html/rfc3986#section-5.3
	{ "file.ext", "file:///c:/folder/file.ext" },
	{ "/file.ext", "file:///file.ext" },
	{ "/folder/file.ext", "file:///folder/file.ext" },
	{ "/c:/folder/file.ext", "file:///c:/folder/file.ext" },
	{ "///c:/folder/file.ext", "file:///c:/folder/file.ext" },
	{ "file:///c:/folder/file.ext", "file:///c:/folder/file.ext" },
	// 	{ "file:///c:/file.ext?", "file:///c:/file.ext?" }, // FIXME
	// 	{ "file:///c:/file.ext?#", "file:///c:/file.ext?#" }, // FIXME
	// 	{ "file:///c:/file.ext#", "file:///c:/file.ext#" }, // FIXME
};

struct URI_REPLACE
{
	const char* Orig;
	const char* NewScheme;
	const char* NewAuthority;
	const char* NewPath;
	const char* NewQuery;
	const char* NewFragment;
	const char* Expected;
};

static const URI_REPLACE sTESTuri_replace[] = 
{
	{ "file:///c:/users/bob/desktop/test.xml", nullptr, nullptr, nullptr, nullptr, "test", "file:///c:/users/bob/desktop/test.xml#test" },
	{ "file://USER/desktop/test.xml", nullptr, nullptr, nullptr, "test", nullptr, "file://USER/desktop/test.xml?test" },
};

typedef void (*fn_t)(unit_test::services& services);
static void thread_proc(unit_test::services& srv, const char* _Name, fn_t _Test, std::exception_ptr* _pException)
{
	*_pException = std::exception_ptr();
	try { _Test(srv); }
	catch (...) { *_pException = std::current_exception(); }
	srv.trace("%s %s", _Name, *_pException == std::exception_ptr() ? "succeeded" : "failed");
}

static void TESTuri_make_absolute(unit_test::services& services, size_t _Start, size_t _NumResolves)
{
	const uri_t sTESTuri_make_absolute_base("http://a/b/c/d;p?q");

	for (size_t i = _Start; i < _NumResolves; i++)
	{
		const auto& t = sTESTuri_make_absolute[i];
		uri_t u(sTESTuri_make_absolute_base, t.Ref); 
		uri_t r(t.Resolved);
		oCHECK(u == r, "fail(%d) (absolute): %s + %s != %s", i, sTESTuri_make_absolute_base.c_str(), t.Ref, t.Resolved);
	}
}

static void TESTuri_make_absolute1a(unit_test::services& services)
{
	TESTuri_make_absolute(services, 0, countof(sTESTuri_make_absolute) / 2);
}

static void TESTuri_make_absolute1b(unit_test::services& services)
{
	const size_t count = countof(sTESTuri_make_absolute) / 2;
	TESTuri_make_absolute(services, count, count);
}

static void TESTuri_make_absolute2(unit_test::services& services)
{
	const uri_t sTESTuri_make_absolute_base2("file://DATA/Test/Scenes/TestTextureSet.xml");

	for (auto i = 0; i < countof(sTESTuri_make_absolute2); i++)
	{
		const auto& t = sTESTuri_make_absolute2[i];
		uri_t u(sTESTuri_make_absolute_base2, t.Ref); 
		uri_t r(t.Resolved);
		oCHECK(u == r, "fail(%d) (absolute2): %s + %s != %s", i, sTESTuri_make_absolute_base2.c_str(), t.Ref, t.Resolved);
	}
}

static void TESTuri_make_absolute3(unit_test::services& services)
{
	const uri_t sTESTuri_make_absolute_base3("file:///c:/folder/file.ext");

	for (auto i = 0; i < countof(sTESTuri_make_absolute3); i++)
	{
		const auto& t = sTESTuri_make_absolute3[i];
		uri_t u(sTESTuri_make_absolute_base3, t.Ref); 
		uri_t r(t.Resolved);
		oCHECK(u == r, "fail(%d) (absolute3): %s + %s != %s", i, sTESTuri_make_absolute_base3.c_str(), t.Ref, t.Resolved);
	}
}

static const URI_RESOLVE sTESTuri_make_relative[] = 
{
	{ "file://DATA/Test/Scenes/TestTextureSet.xml", "TestTextureSet.xml" }, // Should be ""
	{ "file://DATA/Test/Scenes/TestTextureSet.xml#item", "TestTextureSet.xml#item" }, // Should be "#item"
	{ "file://DATA/Test/Scenes/TestTextureSet.xml?query", "TestTextureSet.xml?query" }, // Should be "?query"

	{ "file://DATA/Test/Scenes/test/test/test.xml", "test/test/test.xml" },
	{ "file://DATA/Test/Scenes/test/test.xml", "test/test.xml" },
	{ "file://DATA/Test/Scenes/test.xml", "test.xml" },
	{ "file://DATA/Test/test.xml", "../test.xml" },
	{ "file://DATA/test.xml", "../../test.xml" },
	{ "file://DATA/", "../../" },
};

static void TESTuri_make_relative(unit_test::services& services)
{
	const uri_t sTESTuri_make_relative_base("file://DATA/Test/Scenes/TestTextureSet.xml");

	for (auto i = 0; i < countof(sTESTuri_make_relative); i++)
	{
		const auto& t = sTESTuri_make_relative[i];
		uri_t u(t.Ref);
		u.make_relative(sTESTuri_make_relative_base);
		uri_t r(t.Resolved);
		oCHECK(u == r, "fail(%d) (relative): %s - %s != %s", i, sTESTuri_make_relative_base.c_str(), t.Ref, t.Resolved);
	}
}

static void TESTuri_replace(unit_test::services& services)
{
	for (auto i = 0; i < countof(sTESTuri_replace); i++)
	{
		const auto& t = sTESTuri_replace[i];

		uri_t u(t.Orig);
		u.replace(t.NewScheme, t.NewAuthority, t.NewPath, t.NewQuery, t.NewFragment);
		uri_t e(t.Expected);
		oCHECK(u == e, "fail(%d) (replace) got %s, expected %s", i, u.c_str(), t.Expected);
	}
}

oTEST(oString_uri)
{
	const char* Names[] = 
	{
		"TESTuri_parts",
		"TESTuri_absolute",
		"TESTuri_same_document",
		"TESTuri_make_absolute1a",
		"TESTuri_make_absolute1b",
		"TESTuri_make_absolute2",
		"TESTuri_make_absolute3",
		"TESTuri_make_relative",
		"TESTuri_replace",
	};

	const fn_t Functions[] = 
	{
		TESTuri_parts,
		TESTuri_absolute,
		TESTuri_same_document,
		TESTuri_make_absolute1a,
		TESTuri_make_absolute1b,
		TESTuri_make_absolute2, // 7
		TESTuri_make_absolute3,
		TESTuri_make_relative,
		TESTuri_replace,
	};
	static_assert(countof(Names) == countof(Functions), "array mismatch");

	timer tm;

	std::vector<std::thread> Threads;
	Threads.resize(countof(Names));

	std::vector<std::exception_ptr> Exceptions;
	Exceptions.resize(countof(Names));

	for (size_t i = 0; i < Threads.size(); i++)
		Threads[i] = std::thread(thread_proc, std::ref(srv), Names[i], Functions[i], &Exceptions[i]);

	for (auto& t : Threads)
		t.join();

	for (auto& e : Exceptions)
		if (e != std::exception_ptr())
			std::rethrow_exception(e);

	srv.trace("oString_uri took %.03f sec", tm.seconds());
}