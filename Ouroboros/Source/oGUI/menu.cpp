// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/menu.h>
#include <oCore/assert.h>
#include <oSystem/windows/win_error.h>

namespace ouro { namespace gui { namespace menu { 

#if 0
// not in use (yet?)
static int find_position(menu_handle parent, int item)
{
	const int n = num_items(parent);
	for (int i = 0; i < n; i++)
	{
		int ID = GetMenuItemID(parent, i);
		if (ID == item)
			return i;
	}
	return invalid;
}
#endif

static int find_position(menu_handle parent, menu_handle submenu)
{
	const int n = GetMenuItemCount((HMENU)parent);
	for (int i = 0; i < n; i++)
	{
		menu_handle hSubmenu = (menu_handle)GetSubMenu((HMENU)parent, i);
		if (hSubmenu == submenu)
			return i;
	}
	return -1;
}

#if oHAS_oASSERT
// Returns true if the specified menu contains all IDs [first,last]
static bool contains_range(menu_handle m, int item_range_first, int item_range_last)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_ID;

	int nFound = 0;
	const int n = num_items(m);
	for (int i = 0; i < n; i++)
	{
		UINT uID = GetMenuItemID((HMENU)m, i);
		if (uID == ~0u)
			return false;
		int ID = (int)uID;
		if (ID >= item_range_first && ID <= item_range_last)
			nFound++;
	}

	return nFound == (item_range_last - item_range_first + 1);
}
#endif

char* get_text_by_position(char* out_text, size_t out_text_size, menu_handle m, int item_position)
{
	if (!GetMenuStringA((HMENU)m, item_position, out_text, (int)out_text_size, MF_BYPOSITION))
		return nullptr;
	return out_text;
}

menu_handle make_menu(bool top_level)
{
	return (menu_handle)(top_level ? CreateMenu() : CreatePopupMenu());
}

void unmake_menu(menu_handle m)
{
	if (IsMenu((HMENU)m))
		oVB(DestroyMenu((HMENU)m));
}

void attach(window_handle _hWindow, menu_handle m)
{
	oVB(SetMenu((HWND)_hWindow, (HMENU)m));
}

int num_items(menu_handle m)
{
	return GetMenuItemCount((HMENU)m);
}

void append_submenu(menu_handle parent, menu_handle submenu, const char* text)
{
	oVB(AppendMenu((HMENU)parent, MF_STRING|MF_POPUP, (UINT_PTR)submenu, text));
}

void remove_submenu(menu_handle parent, menu_handle submenu)
{
	int p = find_position(parent, submenu);
	if (p != -1 && !RemoveMenu((HMENU)parent, p, MF_BYPOSITION))
	{
		DWORD hr = GetLastError();
		if (GetLastError() != ERROR_RESOURCE_TYPE_NOT_FOUND)
			oV(hr);
	}
}

void replace_item_with_submenu(menu_handle parent, int item, menu_handle submenu)
{
	mstring text;
	if (!get_text(text, parent, item))
		oThrow(std::errc::invalid_argument, "");

	oVB(RemoveMenu((HMENU)parent, item, MF_BYCOMMAND));
	oVB(InsertMenu((HMENU)parent, item, MF_STRING|MF_POPUP, (UINT_PTR)submenu, text));
}

void replace_submenu_with_item(menu_handle parent, menu_handle submenu, int item, bool enabled)
{
	int p = find_position(parent, submenu);
	oAssert(p != -1, "the specified submenu is not under the specified parent menu");
	
	mstring text;	
	if (!get_text_by_position(text, text.capacity(), parent, p))
		oThrow(std::errc::invalid_argument, "");

	oVB(DeleteMenu((HMENU)parent, p, MF_BYPOSITION));

	UINT uFlags = MF_BYPOSITION|MF_STRING;
	if (!enabled)
		uFlags |= MF_GRAYED;

	oVB(InsertMenu((HMENU)parent, p, uFlags, (UINT_PTR)item, text.c_str()));
}

void append_item(menu_handle parent, int item, const char* text)
{
	oVB(AppendMenu((HMENU)parent, MF_STRING, (UINT_PTR)item, text));
}

void remove_item(menu_handle parent, int item)
{
	if (!RemoveMenu((HMENU)parent, item, MF_BYCOMMAND))
	{
		DWORD hr = GetLastError();
		if (GetLastError() != ERROR_RESOURCE_TYPE_NOT_FOUND)
			oV(hr);
	}
}

void remove_all(menu_handle m)
{
	int n = GetMenuItemCount((HMENU)m);
	while (n)
	{
		DeleteMenu((HMENU)m, n-1, MF_BYPOSITION);
		n = GetMenuItemCount((HMENU)m);
	}
}

void append_separator(menu_handle parent)
{
	oVB(AppendMenu((HMENU)parent, MF_SEPARATOR, 0, nullptr));
}

void check(menu_handle m, int item, bool checked)
{
	oAssert(item >= 0, "");
	if (-1 == CheckMenuItem((HMENU)m, static_cast<unsigned int>(item), MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED)))
		oAssert(false, "MenuItemID not found in the specified menu");
}

bool checked(menu_handle m, int item)
{
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	oAssert(item >= 0, "");
	if (!GetMenuItemInfo((HMENU)m, static_cast<unsigned int>(item), FALSE, &mii))
		return false;

	if (mii.fState & MFS_CHECKED)
		return true;

	return false;
}

void check_radio(menu_handle m, int item_range_first, int item_range_last, int check_item)
{
	// CheckMenuRadioItem returns false if the menu is wrong, but doesn't set a 
	// useful last error (S_OK is returned when I ran into this) so add our own
	// check here.
	oAssert(num_items(m) >= (item_range_last-item_range_first+1), "A radio range was specified that is larger than the number of elements in the list (menu count=%d, range implies %d items)", num_items(m), (item_range_last-item_range_first+1));
	oAssert(contains_range(m, item_range_first, item_range_last), "The specified menu 0x%p does not include the specified range [%d,%d] with selected %d. Most API works with an ancestor menu but this requires the immediate parent, so if the ranges look correct check the specified m.", m, item_range_first, item_range_last, check_item);
	oVB(CheckMenuRadioItem((HMENU)m, item_range_first, item_range_last, check_item, MF_BYCOMMAND));
}

int checked_radio(menu_handle m, int item_range_first, int item_range_last)
{
	for (int i = item_range_first; i <= item_range_last; i++)
		if (checked(m, i))
			return i;
	return -1;
}

void enable(menu_handle m, int item, bool enabled)
{
	oAssert(item >= 0, "");
	if (-1 == EnableMenuItem((HMENU)m, static_cast<unsigned int>(item), MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED)))
		oAssert(false, "MenuItemID not found in the specified menu");
}

bool enabled(menu_handle m, int item)
{
	MENUITEMINFO mii;
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	oAssert(item >= 0, "");
	oVB(GetMenuItemInfo((HMENU)m, static_cast<unsigned int>(item), FALSE, &mii));
	if (mii.fState & (MF_GRAYED|MF_DISABLED))
		return false;
	return true;
}

char* get_text(char* out_text, size_t out_text_size, menu_handle m, int item)
{
	if (!GetMenuStringA((HMENU)m, item, out_text, (int)out_text_size, MF_BYCOMMAND))
		return nullptr;
	return out_text;
}

char* get_text(char* out_text, size_t out_text_size, menu_handle parent, menu_handle submenu)
{
	int pos = find_position(parent, submenu);
	if (pos == -1)
		return nullptr;
	return get_text_by_position(out_text, out_text_size, parent, pos);
}

}}}