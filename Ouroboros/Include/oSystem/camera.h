// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Abstraction for direct-show cameras.

#pragma once
#include <functional>
#include <memory>
#include <oSurface/surface.h>

namespace ouro {

class camera
{
public:
	enum format
	{
		unknown = surface::format::unknown,
		rgb565 = surface::format::b5g5r5a1_unorm,
		rgb24 = surface::format::b8g8r8_unorm,
		argb32 = surface::format::b8g8r8a8_unorm,
		rgb32 = surface::format::b8g8r8x8_unorm,
	};

	struct mode
	{
		mode()
			: dimensions(0, 0)
			, format(unknown)
			, bit_rate(0)
		{}

		int2 dimensions;
		enum format format;
		int bit_rate;
	};

	struct const_mapped
	{
		const void* data;
		unsigned int row_pitch;
		unsigned int frame;
	};

	virtual const char* name() const = 0;
	virtual unsigned int id() const = 0;
	virtual float fps() const = 0;
	virtual mode get_mode() const = 0;
	virtual void set_mode(const mode& _Mode) = 0;
	virtual bool capturing() const = 0;
	virtual void capturing(bool _Capturing) = 0;

	// Return true from _Enumerator to continue, or false to abort enumeration.
	virtual void enumerate_modes(const std::function<bool(const mode& _Mode)>& _Enumerator) const = 0;

	// returns mode() if none found
	virtual mode find_closest_matching(const mode& _Mode) const = 0;

	virtual bool map_const(const_mapped* _pMapped) const = 0;
	virtual void unmap_const() const = 0;

	// Cameras aren't created so much as retrieved using this enumerator.
	static void enumerate(const std::function<bool(std::shared_ptr<camera> _Camera)>& _Enumerator);
};

}
