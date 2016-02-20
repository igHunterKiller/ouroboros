// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace windows {

enum class version
{
	unknown,
	win2000,
	xp,
	xp_sp1,
	xp_sp2,
	xp_sp3,
	xp_pro_64bit,
	server_2003,
	home_server,
	server_2003r2,
	vista,
	server_2008,
	vista_sp1,
	server_2008_sp1,
	vista_sp2,
	server_2008_sp2,
	win7,
	server_2008r2,
	win7_sp1,
	server_2008r2_sp1,
	win8,
	server_2012,
	win8_1,
	server_2012_sp1,
	win10,

	count,
};

version get_version();

}}
