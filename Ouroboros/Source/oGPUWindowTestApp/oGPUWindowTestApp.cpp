// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oMath/primitive.h>

#include <oSystem/reporting.h>
#include <oSystem/thread_traits.h>

#include <oGUI/msgbox.h>
#include <oGUI/msgbox_reporting.h>
#include <oGUI/Windows/win_gdi_bitmap.h>
#include <oGUI/menu.h>
#include <oGUI/enum_radio_handler.h>
#include <oGUI/window.h>
#include "../about_ouroboros.h"

#include <oConcurrency/backoff.h>

#include <oGPU/gpu.h>

#include "resource.h"

using namespace ouro;
using namespace ouro::gui;
using namespace windows::gdi;

namespace ui {

namespace menu { enum value
{
	file,
	edit,
	view,
	view_style,
	view_state,
	help,
	count,
	top_level,
};}

struct menu_hierarchy
{
	menu::value parent;
	menu::value child;
	const char* name;
};

static const menu_hierarchy s_menu_hierarchy[] = 
{
	{ menu::top_level, menu::file,       "&File"         },
	{ menu::top_level, menu::edit,       "&Edit"         },
	{ menu::top_level, menu::view,       "&View"         },
	{ menu::view,      menu::view_style, "Border Style"  },
	{ menu::view,      menu::view_state, "&Window State" },
	{ menu::top_level, menu::help,       "&Help"         },
};
match_array_e(s_menu_hierarchy, menu::value);

namespace menu_item { enum value
{
	file_exit,

	view_style_first,
	view_style_last = view_style_first + (int)window_style::count - 1,

	view_state_first,
	view_state_last = view_state_first + (int)window_state::count - 1,

	view_exclusive,

	help_about,
};}

namespace hotkey { enum value
{
	toggle_ui_mode,
	default_style,
	toggle_fullscreen,
};}

static const ouro::hotkey s_hotkeys[] =
{
	// reset style
	{ key::f2,    key_modifier::none, hotkey::toggle_ui_mode    },
	{ key::f3,    key_modifier::none, hotkey::default_style     },
	{ key::enter, key_modifier::alt,  hotkey::toggle_fullscreen },
};

}

class gpu_test_app
{
public:
	gpu_test_app();
	~gpu_test_app();

	void run();

// _____________________________________________________________________________
private:
	std::shared_ptr<window> gpu_win_;
	std::shared_ptr<window> app_win_;
	std::shared_ptr<about> about_;
	ref<gpu::device> dev_;
	gpu::vbv first_tri_;
	uint32_t clear_color_index_;
	bool running_;

// _____________________________________________________________________________
private:
	std::thread ui_thread_;
	std::array<menu_handle, ui::menu::count> menus_;
	menu::enum_radio_handler erh_;
	window_state pre_fullscreen_state_;
	bool standalone_mode_;
	bool allow_standalone_mode_change_;

	void ui_run();

	// handlers: events should be handled separately but input 
	// should funnel to the same handler.
	void on_event_app(const window::basic_event& evt);
	void on_event_gpu(const window::basic_event& evt);
	void on_input(const input_t& inp);
	void on_menu(const input_t& inp);
	void on_hotkey(const input_t& inp);

	// UI elements: these don't showcase the gpu api per-sae, but
	// allow its features to be shown integrated with OS UI drawing
	void create_menus(const window::create_event& evt);
	void enable_status_bar_styles(bool enabled);
	void check_state(window_state state);
	void check_style(window_style style);

	// GPU window inside UI window v. GPU window standalone
	// window::shape() must be called from the window's thread, so when in standalone
	// mode it is necessary to cache the gpu thread's shape before redirecting the 
	// operation to the UI thread
	void set_standalone_mode_internal(bool enabled, const window_shape& gpu_shape = window_shape());
	void set_standalone_mode(bool enabled);
	void toggle_fullscreen_coop(window* win);
};

gpu_test_app::gpu_test_app()
	: dev_(nullptr)
	, clear_color_index_(0)
	, running_(false)
	, standalone_mode_(false)
	, allow_standalone_mode_change_(true)
{
	// create the render display window
	{
		window::init_t i;
		i.title = "GPU Window";
		i.icon = (icon_handle)load_icon(IDI_APPICON);
		i.alt_f4_closes = true;
		i.on_event = std::bind(&gpu_test_app::on_event_gpu, this, std::placeholders::_1);
		i.on_input = std::bind(&gpu_test_app::on_input, this, std::placeholders::_1);
		i.shape.state = window_state::hidden;
		i.shape.style = window_style::borderless;
		i.shape.client_size = int2(256, 256);
		i.shape.client_position = int2(0, 0);
		gpu_win_ = window::make(i);
		gpu_win_->new_hotkeys(ui::s_hotkeys);
	}

	// create the render device and indexed resources
	{
		gpu::device_init i;
		i.enable_driver_reporting = true;
		dev_ = gpu::new_device(i, gpu_win_.get());
	}

	// create a scene to render
	{
		auto first_tri = primitive::first_tri_mesh(primitive::tessellation_type::solid);
		first_tri_ = dev_->new_vbv("first triangle", sizeof(float3), 3, first_tri.positions);
		dev_->new_rso("root signature", gpu::root_signature_desc(0, nullptr, 0, nullptr, nullptr), 0);
		dev_->new_pso("default", gpu::pipeline_state_desc(gpu::basic::VSpass_through_pos, gpu::basic::PSwhite, mesh::basic::pos, gpu::basic::opaque, gpu::basic::front_face, gpu::basic::no_depth_stencil), 0);
	}

	// start the ui thread
	running_ = false;
	ui_thread_ = std::thread(std::bind(&gpu_test_app::ui_run, this));

	// block until the ui thread is initialized
	backoff bo;
	while (!running_)
		bo.pause();

	// attach gpu window to gui window and ensure that's fully processed
	{
		gpu_win_->parent(app_win_);
		gpu_win_->show();
		gpu_win_->flush_messages();
	}
	app_win_->show();
}

gpu_test_app::~gpu_test_app()
{
	ui_thread_.join();
	dev_->del_vbv(first_tri_);
}

void gpu_test_app::create_menus(const window::create_event& evt)
{
	for (auto& m : menus_)
		m = menu::make_menu();

	for (const auto& h : ui::s_menu_hierarchy)
		menu::append_submenu(h.parent == ui::menu::top_level ? evt.menu : menus_[h.parent], menus_[h.child], h.name);

	// File menu
	menu::append_item(menus_[ui::menu::file], ui::menu_item::file_exit, "E&xit\tAlt+F4");

	// Edit menu
	// (nothing yet)

	// View menu
	menu::append_enum_items(window_style::count, menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, evt.shape.style);
	enable_status_bar_styles(true);

	erh_.add         (menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, [=](int border_style) { app_win_->style((window_style)border_style); });
	menu::check_radio(menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, ui::menu_item::view_style_first + (int)window_style::sizable_with_menu);

	menu::append_enum_items(window_state::count, menus_[ui::menu::view_state], ui::menu_item::view_state_first, ui::menu_item::view_state_last, evt.shape.state);
	erh_.add               (                     menus_[ui::menu::view_state], ui::menu_item::view_state_first, ui::menu_item::view_state_last, [=](int state) { app_win_->show((window_state)state); });

	menu::append_item(menus_[ui::menu::view], ui::menu_item::view_exclusive, "Fullscreen E&xclusive");

	// Help menu
	menu::append_item(menus_[ui::menu::help], ui::menu_item::help_about, "About...");
}

void gpu_test_app::enable_status_bar_styles(bool enabled)
{
	// enable styles not allowed for render target windows
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::fixed_with_statusbar,            enabled);
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::fixed_with_menu_and_statusbar,   enabled);
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::sizable_with_statusbar,          enabled);
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::sizable_with_menu_and_statusbar, enabled);
}

void gpu_test_app::check_state(window_state state)
{
	menu::check_radio(menus_[ui::menu::view_state], ui::menu_item::view_state_first, ui::menu_item::view_state_last, ui::menu_item::view_state_first + (int)state);
}

void gpu_test_app::check_style(window_style style)
{
	menu::check_radio(menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, ui::menu_item::view_style_first + (int)style);
}

void gpu_test_app::set_standalone_mode_internal(bool enabled, const window_shape& gpu_shape)
{
	if (enabled)
	{
		// initialize the gpu window's shape from the app's shape
		window_shape new_gpu_shape(app_win_->shape());
		app_win_->hide();

		// Status bar isn't allowed in this mode
		if (has_statusbar(new_gpu_shape.style))
			new_gpu_shape.style = window_style((int)new_gpu_shape.style - 2);

		gpu_win_->parent(nullptr);
		gpu_win_->shape(new_gpu_shape);
		gpu_win_->focus(true);
	}

	else
	{
		auto old_gpu_shape = gpu_shape;
		auto new_app_shape = app_win_->shape();

		// match the gpu window size
		new_app_shape.client_position = old_gpu_shape.client_position;
		new_app_shape.client_size = old_gpu_shape.client_size;

		// position the standalone gpu window to client space
		window_shape new_gpu_shape;
		new_gpu_shape.style = window_style::borderless;
		new_gpu_shape.client_position = int2(0, 0);
		new_gpu_shape.client_size = new_app_shape.client_size;
		gpu_win_->shape(new_gpu_shape);

		// reparent and resize everything hidden
		gpu_win_->parent(app_win_);
		app_win_->shape(new_app_shape);

		// now show in the original state
		app_win_->show(old_gpu_shape.state);
		app_win_->focus(true);
	}

	standalone_mode_ = enabled;
}

void gpu_test_app::set_standalone_mode(bool enabled)
{
	if (!allow_standalone_mode_change_)
		return;

	if (app_win_->is_window_thread())
		set_standalone_mode_internal(enabled);
	else
	{
		auto shape = gpu_win_->shape();
		app_win_->dispatch( [=] { set_standalone_mode_internal(enabled, shape); } );
	}
}

void gpu_test_app::toggle_fullscreen_coop(window* win)
{
	if (win->state() != window_state::fullscreen)
	{
		pre_fullscreen_state_ = win->state();
		allow_standalone_mode_change_ = false;
		win->state(window_state::fullscreen);
	}
	else
	{
		allow_standalone_mode_change_ = true;
		win->state(pre_fullscreen_state_);
	}
}

void gpu_test_app::on_event_gpu(const window::basic_event& evt)
{
	if (!gpu_win_ || !dev_)
		return;

	switch (evt.type)
	{
		case event_type::sizing:
		{
			dev_->on_window_resizing();
			break;
		}

		case event_type::sized:
		{
			dev_->on_window_resized();
			break;
		}

		case event_type::closing:
			running_ = false;
			break;
	}
}

void gpu_test_app::on_event_app(const window::basic_event& evt)
{
	switch (evt.type)
	{
		case event_type::timer:
			if (evt.as_timer().context == (uintptr_t)&clear_color_index_)
				clear_color_index_ = (clear_color_index_ + 1) & 0x1;
			break;

		case event_type::sized:
		{
			if (gpu_win_ && app_win_)
				gpu_win_->client_size(app_win_->client_size());
			check_state(evt.as_shape().shape.state);
			check_style(evt.as_shape().shape.style);
			break;
		}

		case event_type::creating:
			create_menus(evt.as_create());
			break;

		case event_type::closing:
			running_ = false;
			break;
	}
}

void gpu_test_app::on_input(const input_t& inp)
{
	// reminder this is attached to both windows, so messages from either
	// get pumped through here from either thread.

	switch (inp.type)
	{
		case input_type::menu:   on_menu(inp);   break;
		case input_type::hotkey: on_hotkey(inp); break;
	}
}

void gpu_test_app::on_menu(const input_t& inp)
{
	switch (inp.menu.id)
	{
		case ui::menu_item::file_exit:
			running_ = false;
			break;
		case ui::menu_item::help_about:
			about_->show_modal(app_win_);
			break;
		case ui::menu_item::view_exclusive:
		{
			const bool checked = menu::checked(menus_[ui::menu::view], inp.menu.id);
			menu::check(menus_[ui::menu::view], inp.menu.id, !checked);
			app_win_->set_status_text(1, "Fullscreen %s", !checked ? "exclusive" : "cooperative");
			break;
		}
		default:
			erh_.on_input(inp);
			break;
	}
}

void gpu_test_app::on_hotkey(const input_t& inp)
{
	switch (inp.hotkey.id)
	{
		case ui::hotkey::toggle_ui_mode:
			set_standalone_mode(!standalone_mode_);
			break;

		case ui::hotkey::default_style:
		{
			if (!standalone_mode_)
			{
				if (app_win_->state() == window_state::fullscreen)
					app_win_->state(window_state::restored);
				app_win_->style(window_style::sizable_with_menu_and_statusbar);
			}
			break;
		}

		case ui::hotkey::toggle_fullscreen:
		{
			if (!standalone_mode_)
				toggle_fullscreen_coop(app_win_.get());
			else
			{
				const bool checked = menu::checked(menus_[ui::menu::view], ui::menu_item::view_exclusive);
				if (checked)
				{
					auto mode = dev_->get_present_mode();
					auto new_mode = mode == gpu::present_mode::fullscreen_exclusive ? gpu::present_mode::windowed : gpu::present_mode::fullscreen_exclusive;
					dev_->set_present_mode(new_mode);
					allow_standalone_mode_change_ = new_mode != gpu::present_mode::fullscreen_exclusive;
				}
				else
					toggle_fullscreen_coop(gpu_win_.get());
			}
			break;
		}
	}
}

void gpu_test_app::ui_run()
{
	core_thread_traits::begin_thread("UI Thread");

	try
	{
		// create the UI window
		{
			window::init_t i;
			i.title = "App Window";
			i.icon = (icon_handle)load_icon(IDI_APPICON);
			i.alt_f4_closes = true;
			i.on_event = std::bind(&gpu_test_app::on_event_app, this, std::placeholders::_1);
			i.on_input = std::bind(&gpu_test_app::on_input, this, std::placeholders::_1);
			i.shape.state = window_state::hidden;
			i.shape.style = window_style::sizable_with_menu_and_statusbar;
			i.shape.client_size = int2(256, 256);
			app_win_ = window::make(i);

			// initialize some extra UI features
			{
				app_win_->start_timer((uintptr_t)&clear_color_index_, 1000);
				app_win_->new_hotkeys(ui::s_hotkeys);
			
				const int s_sections[] = { 120, -1 };
				app_win_->set_num_status_sections(s_sections);
				app_win_->set_status_text(0, "F3 for default style");
				app_win_->set_status_text(1, "Fullscreen cooperative");

				oDECLARE_ABOUT_INFO(inf, load_icon(IDI_APPICON));
				about_ = about::make(inf);
			}
		}

		running_ = true;
		while (running_)
			app_win_->flush_messages();
	}

	catch (std::exception& e)
	{
		msgbox(msg_type::info, nullptr, "oGPUWindowTestApp UI Thread", "ERROR\n%s", e.what());
	}

	core_thread_traits::end_thread();
}

void gpu_test_app::run()
{
	static const uint32_t s_clear_colors[2] = { color::red, color::lime };

	try
	{
		running_ = true;
		while (running_)
		{
			gpu_win_->flush_messages();

			auto cl = dev_->immediate();
			cl->clear_rtv(dev_->get_presentation_rtv(), s_clear_colors[clear_color_index_]);
			cl->set_rtv(dev_->get_presentation_rtv());
			cl->set_pso(0);
			cl->set_vertices(0, 1, &first_tri_);
			cl->draw(3);

			dev_->present();
		}
	}
	catch (std::exception& e)
	{
		msgbox(msg_type::info, nullptr, "oGPUWindowTestApp Main Thread", "ERROR\n%s", e.what());
	}
}

int main(int argc, const char* argv[])
{
	reporting::set_prompter(prompt_msgbox);
	gpu_test_app app;
	app.run();
	return 0;
}
