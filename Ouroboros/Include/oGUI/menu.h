// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// api for creating GUI menus

// Menu: A handle that contains menu items and submenus
// Item: an item in a menu that has an ID but is accessed relative to its 
// parent menu.

#pragma once
#include <oGUI/oGUI.h>
#include <oString/fixed_string.h>

namespace ouro { namespace gui { namespace menu {

// If top_level is true the returned menu_handle can be associated with a window. 
// If false a "popup" menu will be created fit for use as a submenu or right-click 
// context menu.
menu_handle make_menu(bool top_level = false);

// This only needs to be called on menu_handle with no parents such as those 
// detached from a window or parent menu. If an menu_handle is attached then the 
// parent manages the lifetime.
void unmake_menu(menu_handle m);

// Transfer ownership of a menu's lifetime to a window and show it. Setting a null 
// menu will detach the menu associated with the window and client code is responsible 
// for its lifetime.
void attach(window_handle w, menu_handle m);

// Returns the number of items in the menu.
int num_items(menu_handle m);

// The parent menu can be a top-level menu or not. Once a submenu is attached its 
// lifetime is managed by the parent menu.
void append_submenu(menu_handle parent, menu_handle submenu, const char* text);

// Detaches submenu without destroying it.
void remove_submenu(menu_handle parent, menu_handle submenu);

// Removes item and inserts submenu at the same location. This is primarily intended 
// for code to convert a disabled, empty submenu to an enabled active menu to be populated.
void replace_item_with_submenu(menu_handle parent, int item, menu_handle submenu);

// Converts submenu into an item in the parent menu. By default it is created disabled 
// because the primary use of this utility is to disable the top-level entry for a submenu 
// when it has zero entries.
void replace_submenu_with_item(menu_handle parent, menu_handle submenu, int item, bool enabled = false);

// append/remove an item from parent.
void append_item(menu_handle parent, int item, const char* text);
void remove_item(menu_handle parent, int item);

// All items are removed and any submenus destroyed. Take care not to use dangling handles 
// to submenus once this is called.
void remove_all(menu_handle parent);

void append_separator(menu_handle parent);

void enable(menu_handle m, int item, bool enabled = true);
bool enabled(menu_handle m, int item);

void check(menu_handle m, int item, bool checked = true);
bool checked(menu_handle m, int item);

// Radio API ensures only zero or one is selected in a range. Use check() to zero out a selected one.
void check_radio(menu_handle m, int item_range_first, int item_range_last, int check_item);

// If none are checked this returns invalid.
int checked_radio(menu_handle m, int item_range_first, int item_range_last);

char* get_text(char* out_text, size_t out_text_size, menu_handle m, int item);
template<size_t size> char* get_text(char (&out_text)[size], menu_handle m, int item) { return get_text(out_text, size, m, item); }
template<size_t capacity> char* get_text(fixed_string<char, capacity>& out_text, menu_handle m, int item) { return get_text(out_text, out_text.capacity(), m, item); }

char* get_text(char* out_text, size_t out_text_size, menu_handle m, menu_handle submenu);
template<size_t size> char* get_text(char (&out_text)[size], menu_handle parent, menu_handle submenu) { return get_text(out_text, size, parent, submenu); }
template<size_t capacity> char* get_text(fixed_string<char, capacity>& out_text, menu_handle parent, menu_handle submenu) { return get_text(out_text, out_text.capacity(), parent, submenu); }

// Create a menu populated with all values of an enum in the specified range. This is useful 
// to pair with enum_radio_handler below for quick menu construction for radio selection groups. 
// initial_item will evaluate to
// first_item by default.
template<typename enumT> void append_enum_items(const enumT& num_enum_values, menu_handle m, int first_item, int last_item, int initial_item = -1)
{
	const int nItems = last_item - first_item + 1;
	if (nItems != (int)num_enum_values)
		throw std::system_error(std::errc::no_buffer_space, std::system_category(), "Enum count and first/last menu item indices mismatch");
	for (int i = 0; i < nItems; i++)
		append_item(m, first_item + i, as_string((enumT)i));
	check_radio(m, first_item, last_item, initial_item == -1 ? first_item : first_item + initial_item);
}

}}}
