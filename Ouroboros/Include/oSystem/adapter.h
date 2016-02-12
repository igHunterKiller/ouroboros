// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Interface for working with display adapters (video cards)

#pragma once
#include <oCore/macros.h>
#include <oBase/vendor.h>
#include <oCore/version.h>
#include <oMath/hlsl.h>
#include <oString/fixed_string.h>

namespace ouro { namespace display { class id; } namespace adapter {

class id
{
public:
	id() : handle_(nullptr) {}

	bool operator==(const id& that) const { return handle_ == that.handle_; }
	bool operator!=(const id& that) const { return !(*this == that); }
	operator bool() const { return !!handle_; }

private:
	void* handle_;
};

struct info_t
{
	info_t() : vendor(vendor::unknown) {}

	id id;
	mstring description;
	mstring plugnplay_id;
	version_t version;
	vendor vendor;
	version_t feature_level;
};

typedef bool (*enumerate_fn)(const info_t& info, void* user);

void enumerate(enumerate_fn enumerator, void* user);

// Ouroboros requires a minimum adapter driver version. This returns that 
// version.
version_t minimum_version(const vendor& v);

// Checks that all adapters meet or exceed the minimum version
inline bool all_up_to_date()
{
	bool up_to_date = true;
	enumerate([](const info_t& info, void* user)->bool
	{
		if (info.version < minimum_version(info.vendor))
			*(bool*)user = false;
		return *(bool*)user;
	}, &up_to_date);
	return up_to_date;
}

// Given the specified virtual desktop position, find the adapter responsible
// for drawing to it. In this way a device can be matched with a specific 
// screen's adapter. It will also be verified against the specified minimum or 
// exact version.
info_t find(const int2& virtual_desktop_position, const version_t& min_version = version_t(), bool exact_version = true);
inline info_t find(const version_t& min_version, bool exact_version = true) { return find(int2(oDEFAULT, oDEFAULT), min_version, exact_version); }

info_t find(display::id* display_id);

}}
