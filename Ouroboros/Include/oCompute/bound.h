/**************************************************************************
 * The MIT License                                                        *
 * Copyright (c) 2013 Antony Arciuolo.                                    *
 * arciuolo@gmail.com                                                     *
 *                                                                        *
 * Permission is hereby granted, free of charge, to any person obtaining  *
 * a copy of this software and associated documentation files (the        *
 * "Software"), to deal in the Software without restriction, including    *
 * without limitation the rights to use, copy, modify, merge, publish,    *
 * distribute, sublicense, and/or sell copies of the Software, and to     *
 * permit persons to whom the Software is furnished to do so, subject to  *
 * the following conditions:                                              *
 *                                                                        *
 * The above copyright notice and this permission notice shall be         *
 * included in all copies or substantial portions of the Software.        *
 *                                                                        *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                  *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE *
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION  *
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.        *
 **************************************************************************/
// This header is designed to cross-compile in both C++ and HLSL. This defines
// an axis-aligned bounding box.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oCompute_bound_h
#define oCompute_bound_h

#include <oHLSL/oHLSLMacros.h>
#include <oHLSL/oHLSLMath.h>
#include <oHLSL/oHLSLTypes.h>

#ifdef oHLSL

#else

namespace ouro {

template<typename T>
class bound
{
	// hybrid aabox/sphere class
	// the aabox is the minimal size to store all points and the sphere contains that box
public:
	typedef T element_type;
	typedef TVEC3<element_type> vec3_type;
	typedef TVEC4<element_type> vec4_type;

	bound() { clear(); }
	bound(const TVEC3<T>& _Extents) { extents(_Extents); }
	bound(const TVEC3<T>& _Min, const TVEC3<T>& _Max) { extents(_Min, _Max); }

	bool empty() const { return any(Extents < T(0)); }
	void clear() { Sphere = TVEC4<T>(T(0)); extents(T(-1)); }

	TVEC3<T> center() const { return Sphere.xyz(); }
	void center(const TVEC3<T>& _Center) const { return Sphere = TVEC4<T>(_Position, sphere.w); }

	T radius() const { return sphere.w; }
	void radius(const T& _Radius) { Sphere.w = _Radius; }

	TVEC4<T> sphere() const { return Sphere; }

	TVEC3<T> extents() const { return Extents; }
	void extents(const TVEC3<T>& _Extents) { Extents = _Extents; Sphere.w = length(get_max() - get_min()) / T(2); }
	void extents(const TVEC3<T>& _Min, const TVEC3<T>& _Max) { Sphere.xyz() = (_Max - _Min) / T(2); extents(_Max - Sphere.xyz()); }

	TVEC3<T> size() const { return Extents * 2.0f; }

	TVEC3<T> get_min() const { return center() - Extents; }
	TVEC3<T> get_max() const { return center() + Extents; }

private:
	TVEC4<T> Sphere;
	TVEC3<T> Extents;
};

} // namespace ouro

typedef ouro::bound<float> boundf; typedef ouro::bound<double> boundd;

#endif
#endif
