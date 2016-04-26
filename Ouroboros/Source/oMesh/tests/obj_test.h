// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#pragma once
#include <oMesh/obj.h>
#include <oMesh/mesh.h>
#include <memory>

namespace ouro { namespace tests {

struct obj_test
{
	enum which
	{
		cube,

		count, 
	};

	static std::shared_ptr<obj_test> make(which _Which);

	// Provides a hand-constructed OBJ and its binary equivalents. Use this to 
	// test OBJ handling code.

	virtual mesh::info_t info() const = 0;
	virtual mesh::const_source_t source() const = 0;

	virtual const char* obj_path() const = 0;
	virtual const char* mtl_path() const = 0;
	virtual const mesh::obj::group_t* groups() const = 0;

	// returns the text of an OBJ file as if it had been fread into memory. ('\n'
	// is the only newline here)
	virtual const char* file_contents() const = 0;
};

}}
