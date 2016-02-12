// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A simple about dialog that gets most of its information from the module info.

#pragma once
#include <oGUI/window.h>

namespace ouro {

class about : public basic_window
{
public:
	struct info
	{
		info()
			: icon(nullptr)
			, website(nullptr)
			, issue_site(nullptr)
			, components(nullptr)
			, component_comments(nullptr)
		{}

		// Icon used to represent the app in the about dialog
		ouro::icon_handle icon;

		// A URL that will be clickable in the about dialog under "visit our website"
		const char* website;

		// A URL that will be clickable in the about dialog under "report an issue"
		const char* issue_site;

		// A series of strings that are listed in a listbox, usually libraries or 
		// plugins used by the app. Each should be delimited by '|'. For example:
		// "zlib|libpng|tbb"
		const char* components;

		// A more verbose description is displayed for the selected component, so 
		// this list must be accessible by the same index as the above components.
		const char* component_comments;
	};

	static std::shared_ptr<about> make(const info& _Info);

	virtual void show_modal(const std::shared_ptr<window>& _Parent) = 0;
};

}
