// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oGUI/enum_radio_handler.h>
#include <oCore/assert.h>
#include <oSystem/windows/win_error.h>

namespace ouro { namespace gui { namespace menu { 

void enum_radio_handler::add(menu_handle m, int first_item, int last_item, const callback_t& callback)
{
	entry_t e;
	e.handle = m;
	e.first = first_item;
	e.last = last_item;
	e.callback = callback;

	auto it = std::find_if(callbacks.begin(), callbacks.end(), [&](const entry_t& i)
	{
		return (e.handle == i.handle)
			|| (e.first >= i.first && e.first <= i.last) 
			|| (e.last >= i.first && e.last <= i.last);
	});

	if (it != callbacks.end())
		oThrow(std::errc::invalid_argument, "The specified menu/range has already been registered or overlaps a previously registered range");

	callbacks.push_back(e);
}

void enum_radio_handler::on_input(const input_t& input)
{
	if (input.type == input_type::menu)
	{
		auto it = std::find_if(callbacks.begin(), callbacks.end(), [&](const entry_t& e)
		{
			return (int)input.menu.id >= e.first && (int)input.menu.id <= e.last;
		});

		if (it != callbacks.end())
		{
			check_radio(it->handle, it->first, it->last, input.menu.id);
			it->callback(input.menu.id - it->first);
		}
	}
}

}}}