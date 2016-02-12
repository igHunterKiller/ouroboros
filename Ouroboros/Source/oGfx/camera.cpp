// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/camera.h>
#include <oGfx/film.h>
#include <oMath/matrix.h>

namespace ouro { namespace gfx {

void camera::initialize(class film* film, const uint2& viewport_dimensions, float near, float far)
{
	f = film;
	pov_ = pov_t(viewport_dimensions, oRECOMMENDED_PC_FOVX_RADIANSf, near, far);
}
	
void camera::deinitialize()
{
}

}}
