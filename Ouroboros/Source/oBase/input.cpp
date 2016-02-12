// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oBase/input.h>
#include <oString/stringize.h>

namespace ouro {

const char* as_string(const control_status& s)
{
	switch (s)
	{
		case control_status::activated: return "activated";
		case control_status::deactivated: return "deactivated";
		case control_status::selection_changing: return "selection_changing";
		case control_status::selection_changed: return "selection_changed";
		default: break;
	}
	return "?";
}

const char* as_string(const skeleton_clip::flag& c)
{
	switch (c)
	{
		case skeleton_clip::left: return "left";
		case skeleton_clip::right: return "right";
		case skeleton_clip::top: return "top";
		case skeleton_clip::bottom: return "bottom";
		case skeleton_clip::front: return "front";
		case skeleton_clip::back: return "back";
		default: break;
	}
	return "?";
}

const char* as_string(const skeleton_status& s)
{
	switch (s)
	{
		case skeleton_status::update: return "update";
		case skeleton_status::acquired: return "acquired";
		case skeleton_status::lost: return "lost";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(skeleton_status);

const char* as_string(const skeleton_bone& b)
{
	switch (b)
	{
		case skeleton_bone::hip_center: return "hip_center";
		case skeleton_bone::spine: return "spine";
		case skeleton_bone::shoulder_center: return "shoulder_center";
		case skeleton_bone::head: return "head";
		case skeleton_bone::shoulder_left: return "shoulder_left";
		case skeleton_bone::elbow_left: return "elbow_left";
		case skeleton_bone::wrist_left: return "wrist_left";
		case skeleton_bone::hand_left: return "hand_left";
		case skeleton_bone::shoulder_right: return "shoulder_right";
		case skeleton_bone::elbow_right: return "elbow_right";
		case skeleton_bone::wrist_right: return "wrist_right";
		case skeleton_bone::hand_right: return "hand_right";
		case skeleton_bone::hip_left: return "hip_left";
		case skeleton_bone::knee_left: return "knee_left";
		case skeleton_bone::ankle_left: return "ankle_left";
		case skeleton_bone::foot_left: return "foot_left";
		case skeleton_bone::hip_right: return "hip_right";
		case skeleton_bone::knee_right: return "knee_right";
		case skeleton_bone::ankle_right: return "ankle_right";
		case skeleton_bone::foot_right: return "foot_right";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(skeleton_bone);

const char* as_string(const input_type& t)
{
	switch (t)
	{
		case input_type::unknown: return "unknown";
		case input_type::keypress: return "keypress";
		case input_type::mouse: return "mouse";
		case input_type::pad: return "pad";
		case input_type::skeleton: return "skeleton";
		case input_type::voice: return "voice";
		case input_type::touch: return "touch";
		case input_type::hotkey: return "hotkey";
		case input_type::menu: return "menu";
		case input_type::control: return "control";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(input_type);

}
