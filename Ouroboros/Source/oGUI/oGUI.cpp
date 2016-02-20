// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/oGUI.h>
#include <oMath/hlslx.h>

namespace ouro {

template<> const char* as_string(const alignment::value& _Alignment)
{
	switch (_Alignment)
	{
		case alignment::top_left: return "top_left";
		case alignment::top_center: return "top_center";
		case alignment::top_right: return "top_right";
		case alignment::middle_left: return "middle_left";
		case alignment::middle_center: return "middle_center";
		case alignment::middle_right: return "middle_right";
		case alignment::bottom_left: return "bottom_left";
		case alignment::bottom_center: return "bottom_center";
		case alignment::bottom_right: return "bottom_right";
		case alignment::fit_parent: return "fit_parent";
		case alignment::fit_largest_axis: return "fit_largest_axis";
		case alignment::fit_smallest_axis: return "fit_smallest_axis";
		default: break;
	}
	return "?";
}

oDEFINE_ENUM_TO_STRING(alignment::value);
oDEFINE_ENUM_FROM_STRING(alignment::value);

template<> const char* as_string(const window_state::value& _State)
{
	switch (_State)
	{
		case window_state::invalid: return "invalid";
		case window_state::hidden: return "hidden";
		case window_state::minimized: return "minimized";
		case window_state::restored: return "restored";
		case window_state::maximized: return "maximized"; 
		case window_state::fullscreen: return "fullscreen";
		default: break;
	}
	return "?";
}

oDEFINE_ENUM_TO_STRING(window_state::value);
oDEFINE_ENUM_FROM_STRING(window_state::value);

template<> const char* as_string(const window_style::value& _Style)
{
	switch (_Style)
	{
		case window_style::default_style: return "default_style";
		case window_style::borderless: return "borderless";
		case window_style::dialog: return "dialog";
		case window_style::fixed: return "fixed";
		case window_style::fixed_with_menu: return "fixed_with_menu";
		case window_style::fixed_with_statusbar: return "fixed_with_statusbar";
		case window_style::fixed_with_menu_and_statusbar: return "fixed_with_menu_and_statusbar";
		case window_style::sizable: return "sizable";
		case window_style::sizable_with_menu: return "sizable_with_menu";
		case window_style::sizable_with_statusbar: return "sizable_with_statusbar";
		case window_style::sizable_with_menu_and_statusbar: return "sizable_with_menu_and_statusbar";
		default: break;
	}
	return "?";
}

oDEFINE_ENUM_TO_STRING(window_style::value);
oDEFINE_ENUM_FROM_STRING(window_style::value);

template<> const char* as_string(const window_sort_order::value& _SortOrder)
{
	switch (_SortOrder)
	{
		case window_sort_order::sorted: return "sorted";
		case window_sort_order::always_on_top: return "always_on_top";
		case window_sort_order::always_on_top_with_focus: return "always_on_top_with_focus";
		default: break;
	}
	return "?";
}

oDEFINE_ENUM_TO_STRING(window_sort_order::value);
oDEFINE_ENUM_FROM_STRING(window_sort_order::value);

template<> const char* as_string(const control_type::value& _Control)
{
	switch (_Control)
	{
		case control_type::unknown: return "unknown";
		case control_type::group: return "group";
		case control_type::button: return "button";
		case control_type::checkbox: return "checkbox";
		case control_type::radio: return "radio";
		case control_type::label: return "label";
		case control_type::label_centered: return "label_centered";
		case control_type::hyperlabel: return "hyperlabel"; 
		case control_type::label_selectable: return "label_selectable";
		case control_type::icon: return "icon";
		case control_type::textbox: return "textbox";
		case control_type::textbox_scrollable: return "textbox_scrollable";
		case control_type::floatbox: return "floatbox"; 
		case control_type::floatbox_spinner: return "floatbox_spinner";
		case control_type::combobox: return "combobox"; 
		case control_type::combotextbox: return "combotextbox"; 
		case control_type::progressbar: return "progressbar";
		case control_type::progressbar_unknown: return "progressbar_unknown"; 
		case control_type::tab: return "tab"; 
		case control_type::slider: return "slider";
		case control_type::slider_selectable: return "slider_selectable"; 
		case control_type::slider_with_ticks: return "slider_with_ticks"; 
		case control_type::listbox: return "listbox"; 
		default: break;
	}
	return "?";
}

oDEFINE_ENUM_TO_STRING(control_type::value);
oDEFINE_ENUM_FROM_STRING(control_type::value);

template<> const char* as_string(const event_type::value& _Event)
{
	switch (_Event)
	{
		case event_type::timer: return "timer";
		case event_type::activated: return "activated";
		case event_type::deactivated: return "deactivated";
		case event_type::creating: return "creating";
		case event_type::paint: return "paint";
		case event_type::display_changed: return "display_changed";
		case event_type::moving: return "moving";
		case event_type::moved: return "moved";
		case event_type::sizing: return "sizing";
		case event_type::sized: return "sized";
		case event_type::closing: return "closing";
		case event_type::closed: return "closed";
		case event_type::to_fullscreen: return "to_fullscreen";
		case event_type::from_fullscreen: return "from_fullscreen";
		case event_type::lost_capture: return "lost_capture";
		case event_type::drop_files: return "drop_files";
		case event_type::input_device_changed: return "input_device_changed";
		case event_type::custom_event: return "custom_event";
		default: break;
	}
	return "?";
}

oDEFINE_ENUM_TO_STRING(event_type::value);
oDEFINE_ENUM_FROM_STRING(event_type::value);

int4 resolve_rect(const int4& _Parent, const int4& _UnadjustedChild, alignment::value _Alignment, bool _Clip)
{
	int2 cpos = resolve_default(_UnadjustedChild.xy(), int2(0,0));

	int2 psz = _Parent.zw() - _Parent.xy();
	int2 csz = resolve_default(_UnadjustedChild.zw() - cpos, psz);

	float2 ResizeRatios = (float2)psz / max((float2)csz, float2(0.0001f, 0.0001f));

	alignment::value internalAlignment = _Alignment;

	int2 offset(0, 0);

	switch (_Alignment)
	{
		case alignment::fit_largest_axis:
		{
			const float ResizeRatio = min(ResizeRatios);
			csz = round((float2)csz * ResizeRatio);
			cpos = 0;
			internalAlignment = alignment::middle_center;
			break;
		}

		case alignment::fit_smallest_axis:
		{
			const float ResizeRatio = max(ResizeRatios);
			csz = round((float2)csz * ResizeRatio);
			cpos = 0;
			internalAlignment = alignment::middle_center;
			break;
		}

		case alignment::fit_parent:
			return _Parent;

		default:
			// preserve user-specified offset if there was one separately from moving 
			// around the child position according to internalAlignment
			offset = _UnadjustedChild.xy();
			break;
	}

	int2 code = int2(internalAlignment % 3, internalAlignment / 3);

	if (offset.x == oDEFAULT || code.x == 0) offset.x = 0;
	if (offset.y == oDEFAULT || code.y == 0) offset.y = 0;

	// All this stuff is top-left by default, so adjust for center/middle and 
	// right/bottom

	// center/middle
	if (code.x == 1) cpos.x = (psz.x - csz.x) / 2;
	if (code.y == 1) cpos.y = (psz.y - csz.y) / 2;

	// right/bottom
	if (code.x == 2) cpos.x = _Parent.z - csz.x;
	if (code.y == 2) cpos.y = _Parent.w - csz.y;

	int2 FinalOffset = _Parent.xy() + offset;

	int4 resolved;
	resolved.xy() = cpos;
	resolved.zw() = resolved.xy() + csz;

	resolved.xy() += FinalOffset;
	resolved.zw() += FinalOffset;

	if (_Clip)
	{
		resolved.x = __max(resolved.x, _Parent.x);
		resolved.y = __max(resolved.y, _Parent.y);
		resolved.z = __min(resolved.z, _Parent.z);
		resolved.w = __min(resolved.w, _Parent.w);
	}

	return resolved;
}

}

