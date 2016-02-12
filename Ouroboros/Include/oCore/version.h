// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// encapsulates a software version number: major.minor.build or 
// major.minor.build.revision

#pragma once
#include <cstdint>

namespace ouro {

struct version_t
{
	enum { invalid = 65535 };

	version_t() : major(0), minor(0), build(invalid), revision(invalid) {}
	version_t(uint16_t _major, uint16_t _minor, uint16_t _build = invalid, uint16_t _revision = invalid) : major(_major), minor(_minor), build(_build), revision(_revision) {}

	inline bool valid()                           const            { return major || minor || (build && build != invalid) || (revision && revision != invalid); }
	inline bool operator!()                       const            { return !valid(); }
	inline bool operator< (const version_t& that) const            { return valid() && that.valid() && ((major < that.major) || (major == that.major && minor < that.minor) || (major == that.major && minor == that.minor && build != invalid && build < that.build) || (major == that.major && minor == that.minor && build == that.build && revision != invalid && revision < that.revision)); }
	inline bool operator==(const version_t& that) const            { return valid() && that.valid() && major == that.major && minor == that.minor && build == that.build && revision == that.revision; }
	friend bool operator!=(const version_t& a, const version_t& b) { return !(a == b); }
	friend bool operator>=(const version_t& a, const version_t& b) { return !(a < b); }
	friend bool operator<=(const version_t& a, const version_t& b) { return (a < b) || (a == b); }
	friend bool operator> (const version_t& a, const version_t& b) { return !(a <= b); }

	uint16_t major;
	uint16_t minor;
	uint16_t build;
	uint16_t revision;
};

}
