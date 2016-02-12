// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Wrappers for some commonly loaded file types

#pragma once
#include <oSystem/filesystem.h>
#include <oString/ini.h>
#include <oString/xml.h>

namespace ouro { namespace filesystem {

inline std::unique_ptr<xml> load_xml(const path_t& _Path)
{
	blob a = load(_Path, load_option::text_read);
	return std::unique_ptr<xml>(new xml(_Path, (char*)a.release(), a.deleter()));
}

inline std::unique_ptr<ini> load_ini(const path_t& _Path)
{
	blob a = load(_Path, load_option::text_read);
	return std::unique_ptr<ini>(new ini(_Path, (char*)a.release(), a.deleter()));
}

}}
