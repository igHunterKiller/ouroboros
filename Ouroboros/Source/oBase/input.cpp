// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/input.h>
#include <oString/stringize.h>

namespace ouro {

template<> const char* as_string(const control_status& s)
{
	static const char* s_names[] = 
	{
		"activated",
		"deactivated",
		"selection_changing",
		"selection_changed",
	};
	return detail::enum_as(s, s_names);
}

template<> const char* as_string(const skeleton_clip::flag& c)
{
	switch (c)
	{
		case skeleton_clip::left:   return "left";
		case skeleton_clip::right:  return "right";
		case skeleton_clip::top:    return "top";
		case skeleton_clip::bottom: return "bottom";
		case skeleton_clip::front:  return "front";
		case skeleton_clip::back:   return "back";
		default: break;
	}
	return "?";
}

template<> const char* as_string(const skeleton_status& s)
{
	static const char* s_names[] =
	{
		"update",
		"acquired",
		"lost",
	};
	return detail::enum_as(s, s_names);
}

oDEFINE_TO_FROM_STRING(skeleton_status);

template<> const char* as_string(const skeleton_bone& b)
{
	static const char* s_names[] = 
	{
		"hip_center",
		"spine",
		"shoulder_center",
		"head",
		"shoulder_left",
		"elbow_left",
		"wrist_left",
		"hand_left",
		"shoulder_right",
		"elbow_right",
		"wrist_right",
		"hand_right",
		"hip_left",
		"knee_left",
		"ankle_left",
		"foot_left",
		"hip_right",
		"knee_right",
		"ankle_right",
		"foot_right",
	};
	return detail::enum_as(b, s_names);
}

oDEFINE_TO_FROM_STRING(skeleton_bone);

template<> const char* as_string(const input_type& t)
{
	static const char* s_names[] =
	{
		"unknown",
		"keypress",
		"mouse",
		"pad",
		"skeleton",
		"voice",
		"touch",
		"hotkey",
		"menu",
		"control",
	};
	return detail::enum_as(t, s_names);
}

oDEFINE_TO_FROM_STRING(input_type);

}
