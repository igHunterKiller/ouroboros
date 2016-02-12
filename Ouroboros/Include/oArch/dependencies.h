// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// summary and links to each 3rd-party/external software required for compile.

#pragma once

#define oOURO_WEBSITE "http://code.google.com/p/ouroboros2/"
#define oOURO_SUPPORT "http://code.google.com/p/ouroboros2/issues/list"

// To use, define this macro (all params are strings):
// #define oOURO_DEPENDENCY(lib_name, lib_version, url_home, url_license, desc) <your-impl-here>
#define oOURO_DEPENDENCIES \
	oOURO_DEPENDENCY("Ouroboros",    "",              oOURO_WEBSITE,                                                                   oOURO_SUPPORT,                                                                       "") \
	oOURO_DEPENDENCY("bullet",       "v2.82",         "http://bulletphysics.org/wordpress/",                                           "https://code.google.com/p/bullet/source/browse/tags/bullet-2.82/BulletLicense.txt", "vectormath lib only.") \
	oOURO_DEPENDENCY("calfaq",       "1.0",           "http://www.tondering.dk/claus/calendar.html",                                   "http://www.boost.org/LICENSE_1_0.txt",                                              "") \
	oOURO_DEPENDENCY("ispc_texcomp", "r2",            "https://software.intel.com/en-us/articles/fast-ispc-texture-compressor-update", "https://software.intel.com/en-us/articles/fast-ispc-texture-compressor-update",     "") \
	oOURO_DEPENDENCY("libjpegTurbo", "Version 8b",    "http://libjpeg-turbo.virtualgl.org/",                                           "http://sourceforge.net/p/libjpeg-turbo/code/HEAD/tree/trunk/README-turbo.txt",      "") \
	oOURO_DEPENDENCY("libpng",       "v1.6.6",        "http://libpng.sourceforge.net/index.html",                                      "http://sourceforge.net/p/libpng/code/ci/master/tree/LICENSE",                       "") \
	oOURO_DEPENDENCY("lzma",         "v9.20",         "http://www.7-zip.org/sdk.html",                                                 "http://www.7-zip.org/sdk.html",                                                     "") \
	oOURO_DEPENDENCY("smhasher",     "R150",          "http://code.google.com/p/smhasher/",                                            "http://opensource.org/licenses/mit-license.php",                                    "") \
	oOURO_DEPENDENCY("snappy",       "v1.0.3",        "http://code.google.com/p/snappy/",                                              "http://code.google.com/p/snappy/source/browse/trunk/COPYING",                       "") \
	oOURO_DEPENDENCY("tbb",          "v4.2 update 2", "https://www.threadingbuildingblocks.org/",                                      "https://www.threadingbuildingblocks.org/licensing",                                 "") \
	oOURO_DEPENDENCY("tlsf",         "v3.0",          "http://tlsf.baisoku.org/",                                                      "http://tlsf.baisoku.org/",                                                          "") \
	oOURO_DEPENDENCY("zlib",         "v1.2.5",        "http://www.zlib.net/",                                                          "https://www.threadingbuildingblocks.org/licensing",                                 "")
