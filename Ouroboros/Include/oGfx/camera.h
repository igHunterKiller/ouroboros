// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/pov.h>

namespace ouro { namespace gfx {

class film;
class camera
{
public:
	camera() 
		: f(nullptr)
	{}
	
	~camera() { deinitialize(); }

	camera(camera&& that);
	camera& operator=(camera&& that);

	void initialize(class film* film, const uint2& viewport_dimensions = uint2(0, 0), float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
	
	void deinitialize();

	inline const film* film() const { return f; }
	inline const pov_t* pov() const { return &pov_; }
	inline pov_t* pov() { return &pov_; }

	inline const float4x4& view() const { return pov_.view(); }
	inline void view(const float4x4& view) { pov_.view(view); }
	inline void projection(float fovx, float near, float far) { pov_.projection(fovx, near, far); }
	inline const float4x4& projection() const { return pov_.projection(); }

private:
	class film* f;
	pov_t pov_;

	camera(const camera&);
	const camera& operator=(const camera&);
};
	
}}
