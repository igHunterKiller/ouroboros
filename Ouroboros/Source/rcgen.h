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
// Use this in .rc files along with the common property sheet to enable 
// generation of certain defines from source control.
#pragma once
#ifndef ouro_rcgen_h
#define ouro_rcgen_h

#ifdef oUSE_RCGEN
#include "oRCGen.h"
#else

#ifndef oRC_VERSION_VAL
#define oRC_VERSION_VAL 1,0,0,0
#endif

#ifndef oRC_COMPANY
#define oRC_COMPANY "Company"
#endif

#ifndef oRC_VERSION_STR_MS
#define oRC_VERSION_STR_MS "1.0.0.0"
#endif

#ifndef oRC_PRODUCTNAME
#define oRC_PRODUCTNAME "ProductName"
#endif

#ifndef oRC_COPYRIGHT
#define oRC_COPYRIGHT "Copyright"
#endif

#ifndef oRC_FILENAME
#define oRC_FILENAME "Filename"
#endif

#endif
#endif
