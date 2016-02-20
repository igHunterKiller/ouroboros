// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/version.h>
#include <oCore/snprintf.h>
#include <oCore/stringize.h>

namespace ouro {

template<> char* to_string(char* dst, size_t dst_size, const version_t& ver)
{
	static const char* fmt[] = { "%u", "%u.%u", "%u.%u.%u", "%u.%u.%u.%u" };
	
	int i = 0;
	if (ver.major    == version_t::invalid) return nullptr;
	if (ver.minor    != version_t::invalid) i++;
	if (ver.build    != version_t::invalid) i++;
	if (ver.revision != version_t::invalid) i++;
	return -1 != snprintf(dst, dst_size, "%u.%u.%u", ver.major, ver.minor, ver.build, ver.revision) ? dst : nullptr;
}

template<> bool from_string(version_t* out_ver, const char* src)
{
	uint32_t major=0, minor=version_t::invalid, build=version_t::invalid, revision=version_t::invalid;
	int nscanned = sscanf_s(src, "%u.%u.%u.%u", &major, &minor, &build, &revision);
	if (nscanned < 1) return false;
	out_ver->major    = (uint16_t)major;
	out_ver->minor    = (uint16_t)minor;
	out_ver->build    = (uint16_t)build;
	out_ver->revision = (uint16_t)revision;
	return out_ver->major == major && out_ver->minor == minor && out_ver->build == build && out_ver->revision == revision;
}

}
