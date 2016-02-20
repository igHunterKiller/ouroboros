// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/oGUI.h>
#include <oMath/hlslx.h>

namespace ouro {

template<> const char* as_string(const alignment& a)
{
	static const char* s_names[] =
	{
		"top_left",
		"top_center",
		"top_right",
		"middle_left",
		"middle_center",
		"middle_right",
		"bottom_left",
		"bottom_center",
		"bottom_right",
		"fit_parent",
		"fit_largest_axis",
		"fit_smallest_axis",
	};
	return as_string(a, s_names);
}

oDEFINE_TO_FROM_STRING(alignment)

template<> const char* as_string(const window_state& s)
{
	static const char* s_names[] =
	{
		"invalid",
		"hidden",
		"minimized",
		"restored",
		"maximized", 
		"fullscreen",
	};
	return as_string(s, s_names);
}

oDEFINE_TO_FROM_STRING(window_state)

template<> const char* as_string(const window_style& s)
{
	static const char* s_names[] =
	{
		"default_style",
		"borderless",
		"dialog",
		"fixed",
		"fixed_with_menu",
		"fixed_with_statusbar",
		"fixed_with_menu_and_statusbar",
		"sizable",
		"sizable_with_menu",
		"sizable_with_statusbar",
		"sizable_with_menu_and_statusbar",
	};
	return as_string(s, s_names);
}

oDEFINE_TO_FROM_STRING(window_style)

template<> const char* as_string(const window_sort_order& o)
{
	static const char* s_names[] =
	{
		"sorted",
		"always_on_top",
		"always_on_top_with_focus",
	};
	return as_string(o, s_names);
}

oDEFINE_TO_FROM_STRING(window_sort_order)

template<> const char* as_string(const control_type& c)
{
	static const char* s_names[] =
	{
		"unknown",
		"group",
		"button",
		"checkbox",
		"radio",
		"label",
		"label_centered",
		"hyperlabel", 
		"label_selectable",
		"icon",
		"textbox",
		"textbox_scrollable",
		"floatbox", 
		"floatbox_spinner",
		"combobox", 
		"combotextbox", 
		"progressbar",
		"progressbar_unknown", 
		"tab", 
		"slider",
		"slider_selectable", 
		"slider_with_ticks", 
		"listbox", 
	};
	return as_string(c, s_names);
}

oDEFINE_TO_FROM_STRING(control_type)

template<> const char* as_string(const event_type& e)
{
	static const char* s_names[] =
	{
		"timer",
		"activated",
		"deactivated",
		"creating",
		"paint",
		"display_changed",
		"moving",
		"moved",
		"sizing",
		"sized",
		"closing",
		"closed",
		"to_fullscreen",
		"from_fullscreen",
		"lost_capture",
		"drop_files",
		"input_device_changed",
		"custom_event",
	};
	return as_string(e, s_names);
}

oDEFINE_TO_FROM_STRING(event_type)

int4 resolve_rect(const int4& parent, const int4& unadjusted_child, const alignment& alignment, bool clip)
{
	int2 cpos = resolve_default(unadjusted_child.xy(), int2(0,0));

	int2 psz = parent.zw() - parent.xy();
	int2 csz = resolve_default(unadjusted_child.zw() - cpos, psz);

	float2 resize_ratios = (float2)psz / max((float2)csz, float2(0.0001f, 0.0001f));

	auto internal_alignment = alignment;

	int2 offset(0, 0);

	switch (alignment)
	{
		case alignment::fit_major_axis:
		{
			const float ResizeRatio = min(resize_ratios);
			csz = round((float2)csz * ResizeRatio);
			cpos = 0;
			internal_alignment = alignment::middle_center;
			break;
		}

		case alignment::fit_minor_axis:
		{
			const float ResizeRatio = max(resize_ratios);
			csz = round((float2)csz * ResizeRatio);
			cpos = 0;
			internal_alignment = alignment::middle_center;
			break;
		}

		case alignment::fit_parent:
			return parent;

		default:
			// preserve user-specified offset if there was one separately from moving 
			// around the child position according to internal_alignment
			offset = unadjusted_child.xy();
			break;
	}

	int2 code = int2((int)internal_alignment % 3, (int)internal_alignment / 3);

	if (offset.x == oDEFAULT || code.x == 0) offset.x = 0;
	if (offset.y == oDEFAULT || code.y == 0) offset.y = 0;

	// top-left by default, so adjust for center/middle and right/bottom

	// center/middle
	if (code.x == 1) cpos.x = (psz.x - csz.x) / 2;
	if (code.y == 1) cpos.y = (psz.y - csz.y) / 2;

	// right/bottom
	if (code.x == 2) cpos.x = parent.z - csz.x;
	if (code.y == 2) cpos.y = parent.w - csz.y;

	int2 final_offset = parent.xy() + offset;

	int4 resolved;
	resolved.xy() = cpos;
	resolved.zw() = resolved.xy() + csz;

	resolved.xy() += final_offset;
	resolved.zw() += final_offset;

	if (clip)
	{
		resolved.x = __max(resolved.x, parent.x);
		resolved.y = __max(resolved.y, parent.y);
		resolved.z = __min(resolved.z, parent.z);
		resolved.w = __min(resolved.w, parent.w);
	}

	return resolved;
}

}
