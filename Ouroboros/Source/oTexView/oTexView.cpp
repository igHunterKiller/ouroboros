// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// TODO: this should handle small images (and zoom shrinking of small images)
// more gracefully by rendering to a subset of the scene.

#include <oConcurrency/backoff.h>

#include <oCore/countof.h>

#include <oSystem/filesystem.h>
#include <oSystem/reporting.h>
#include <oSystem/thread_traits.h>

#include <oGUI/enum_radio_handler.h>
#include <oGUI/menu.h>
#include <oGUI/msgbox.h>
#include <oGUI/msgbox_reporting.h>
#include <oGUI/window.h>
#include <oGUI/Windows/win_common_dialog.h>
#include <oGUI/Windows/win_gdi_bitmap.h>
#include "../about_ouroboros.h"

#include <oGPU/gpu.h>

#include <oSurface/codec.h>

#include "resource.h"

using namespace ouro;
using namespace ouro::gui;
using namespace windows::gdi;

static const char* s_app_name = "oTexView";

namespace ui {

namespace status_bar { enum value
{
	info,
};}

namespace menu { enum value
{
	file,
	edit,
	view,
	view_zoom,
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
	{ menu::top_level, menu::file,       "&File" },
	{ menu::top_level, menu::edit,       "&Edit" },
	{ menu::top_level, menu::view,       "&View" },
	{ menu::view,      menu::view_zoom,  "&Zoom" },
	{ menu::top_level, menu::help,       "&Help" },
};
match_array_e(s_menu_hierarchy, menu::value);

namespace menu_item { enum value
{
	file_open,
	file_exit,
	view_zoom_quarter,
	view_zoom_half,
	view_zoom_original,
	view_zoom_double,
	view_zoom_first = view_zoom_quarter,
	view_zoom_last = view_zoom_double,
	help_about,
};}

namespace hotkey { enum value
{
	file_open,
	view_zoom_quarter,
	view_zoom_half,
	view_zoom_original,
	view_zoom_double,
};}

ouro::hotkey s_hotkeys[] =
{
	{ key::o, key_modifier::ctrl, hotkey::file_open          },
	{ key::_1, key_modifier::alt, hotkey::view_zoom_quarter  },
	{ key::_2, key_modifier::alt, hotkey::view_zoom_half     },
	{ key::_3, key_modifier::alt, hotkey::view_zoom_original },
	{ key::_4, key_modifier::alt, hotkey::view_zoom_double   },
};

}

class tex_view
{
public:
	tex_view();
	~tex_view();

	void run();

// _____________________________________________________________________________
private:
	std::shared_ptr<window> gpu_win_;
	std::shared_ptr<window> app_win_;
	std::shared_ptr<about> about_;
	ref<gpu::device> dev_;
	ref<gpu::srv> texture_;
	bool running_;

	surface::info_t image_info_;
	surface::image new_image_;
	volatile bool new_image_ready_;

	void refresh();

// _____________________________________________________________________________
private:
	std::thread ui_thread_;
	std::array<menu_handle, ui::menu::count> menus_;
	menu::enum_radio_handler erh_;

	void ui_run();

	// handlers: events should be handled separately but input 
	// should funnel to the same handler.
	void on_event_app(const window::basic_event& evt);
	void on_event_gpu(const window::basic_event& evt);
	void on_drop(const window::basic_event& evt);
	void on_zoom(int id);
	void on_input(const input_t& inp);
	void on_menu(const input_t& inp);
	void on_hotkey(const input_t& inp);

	void create_menus(const window::create_event& evt);
	void open_file_dialog();
	void open_file(const path_t& p);
};

tex_view::tex_view()
	: dev_(nullptr)
	, texture_(nullptr)
	, running_(false)
	, new_image_ready_(false)
{
	// create the render display window
	{
		window::init_t i;
		i.title = s_app_name;
		i.icon = (icon_handle)load_icon(IDI_APPICON);
		i.alt_f4_closes = true;
		i.on_event = std::bind(&tex_view::on_event_gpu, this, std::placeholders::_1);
		i.on_input = std::bind(&tex_view::on_input, this, std::placeholders::_1);
		i.shape.state = window_state::hidden;
		i.shape.style = window_style::borderless;
		i.shape.client_position = int2(0, 0);
		i.shape.client_size = int2(256, 256);
		gpu_win_ = window::make(i);
		int hk_id = gpu_win_->new_hotkeys(ui::s_hotkeys);
		gpu_win_->set_hotkeys(hk_id);
	}

	// create the render device and indexed resources
	{
		gpu::device_init i;
		i.enable_driver_reporting = true;
		dev_ = gpu::new_device(i, gpu_win_.get());

		dev_->new_rso("root signature", gpu::root_signature_desc(1, &gpu::basic::linear_clamp, 0, nullptr, nullptr), 0);
		dev_->new_pso("fullscreen tri", gpu::pipeline_state_desc(gpu::basic::VSfullscreen_tri, gpu::basic::PStex2d, mesh::basic::pos, gpu::basic::opaque, gpu::basic::front_face, gpu::basic::no_depth_stencil), 0);

		dev_->immediate()->set_rso(0);
		dev_->immediate()->set_pso(0);
	}

	// start the ui thread
	running_ = false;
	ui_thread_ = std::thread(std::bind(&tex_view::ui_run, this));

	// block until the ui thread is initialized
	backoff bo;
	while (!running_)
		bo.pause();
}

tex_view::~tex_view()
{
	filesystem::join();
	ui_thread_.join();
}

void tex_view::create_menus(const window::create_event& evt)
{
	for (auto& m : menus_)
		m = menu::make_menu();

	for (const auto& h : ui::s_menu_hierarchy)
		menu::append_submenu(h.parent == ui::menu::top_level ? evt.menu : menus_[h.parent], menus_[h.child], h.name);

	// File menu
	menu::append_item(menus_[ui::menu::file], ui::menu_item::file_open, "&Open...\tCtrl+O");
	menu::append_item(menus_[ui::menu::file], ui::menu_item::file_exit, "E&xit\tAlt+F4");

	// Edit menu
	// (nothing yet)

	// View menu
	const char* s_zoom_menu[] = 
	{
		"1:4 Quarter\tAlt+1",
		"1:2 Half\tAlt+2",
		"1:1 Original\tAlt+3",
		"2:1 Double\tAlt+4",
	};
	match_array(s_zoom_menu, ui::menu_item::view_zoom_last-ui::menu_item::view_zoom_first+1);

	for (int i = ui::menu_item::view_zoom_first; i <= ui::menu_item::view_zoom_last; i++)
	{
		menu::append_item(menus_[ui::menu::view_zoom], i, s_zoom_menu[i - ui::menu_item::view_zoom_first]);
		menu::enable(menus_[ui::menu::view_zoom], i, false);
	}

	// Help menu
	menu::append_item(menus_[ui::menu::help], ui::menu_item::help_about, "About...");
}

void tex_view::open_file_dialog()
{
	const char* s_supported_formats = 
		"Supported Image Types|*.bmp;*.dds;*.jpg;*.png;*.psd;*.tga" \
		"|All Files Types|*.*"                                      \
		"|Bitmap Files|*.bmp"                                       \
		"|DDS Files|*.dds"                                          \
		"|JPG/JPEG Files|*.jpg"                                     \
		"|PNG Files|*.png"                                          \
		"|Photoshop Files|*.psd"                                    \
		"|Targa Files|*.tga";

	path_t p(filesystem::data_path());
	if (windows::common_dialog::open_path(p, "Open Texture", s_supported_formats, (HWND)app_win_->native_handle()))
		open_file(p);
}

void tex_view::open_file(const path_t& p)
{
	if (!app_win_->is_window_thread())
		throw std::invalid_argument("this should run on the UI thread");

	if (surface::file_format::unknown == surface::get_file_format(p))
	{
		msgbox(msg_type::info, app_win_->native_handle(), s_app_name, "Unsupported file type %s", p.c_str());
		return;
	}

	try
	{
		bool first_image = image_info_.format == surface::format::unknown;

		// load the image and decode into a GPU-compatible format
		{
			auto decoded = surface::decode(filesystem::load(p));
			image_info_ = decoded.info();

			if (is_texture(image_info_.format))
				new_image_ = std::move(decoded);
			else
			{
				auto converted = decoded.convert(surface::format::b8g8r8a8_unorm);
				new_image_ = std::move(converted);
			}
		}

		if (first_image)
		{
			for (int i = ui::menu_item::view_zoom_first; i <= ui::menu_item::view_zoom_last; i++)
				menu::enable(menus_[ui::menu::view_zoom], i, true);
		}

		// reload the texture being displayed
		app_win_->set_title("%s - %s", s_app_name, p.c_str());
		app_win_->set_status_text(ui::status_bar::info, "%u x %u %s", image_info_.dimensions.x, image_info_.dimensions.y, as_string(image_info_.format));
		
		// don't allow zoom until there's image data
		on_zoom(ui::menu_item::view_zoom_original);

		// this shouldn't happen in practice, but just in case wait for any
		// prior image to be consumed by the main thread.
		backoff bo;
		while (new_image_ready_)
			bo.pause();

		new_image_ready_ = true;
	}

	catch (std::exception& e)
	{
		msgbox(msg_type::info, app_win_->native_handle(), s_app_name, "Opening %s failed:\n%s", p.c_str(), e.what());
	}
}

void tex_view::on_event_gpu(const window::basic_event& evt)
{
	if (!gpu_win_)
		return;

	switch (evt.type)
	{
		case event_type::sizing:
		{
			if (dev_)
				dev_->on_window_resizing();
			break;
		}

		case event_type::sized:
		{
			if (dev_)
			{
				dev_->on_window_resized();
				refresh();
			}
			break;
		}

		// redirect all others to the ui handler (still on gpu thread though)
		default:
			on_event_app(evt);
			break;
	}
}

void tex_view::on_event_app(const window::basic_event& evt)
{
	switch (evt.type)
	{
		case event_type::sized:
		{
			if (gpu_win_ && app_win_)
				gpu_win_->client_size(app_win_->client_size());
			break;
		}

		case event_type::creating:
			create_menus(evt.as_create());
			break;

		case event_type::closing:
			running_ = false;
			break;
		
		case event_type::drop_files:
			on_drop(evt);
			break;
	}
}

void tex_view::on_drop(const window::basic_event& evt)
{
	if (evt.type != event_type::drop_files)
		return;
	if (evt.as_drop().num_paths)
		open_file(path_t(evt.as_drop().paths[0]));
}

void tex_view::on_zoom(int item)
{
	if (image_info_.format == surface::format::unknown 
		|| item < ui::menu_item::view_zoom_first 
		|| item > ui::menu_item::view_zoom_last
		|| item == menu::checked_radio(menus_[ui::menu::view_zoom], ui::menu_item::view_zoom_first, ui::menu_item::view_zoom_last))
		return;

	menu::check_radio(menus_[ui::menu::view_zoom], ui::menu_item::view_zoom_first, ui::menu_item::view_zoom_last, item);

	int2 new_size = image_info_.dimensions.xy();
	switch (item)
	{
		case ui::menu_item::view_zoom_quarter: new_size /= 4; break;
		case ui::menu_item::view_zoom_half:    new_size /= 2; break;
		case ui::menu_item::view_zoom_double:  new_size *= 2; break;
		default: break;
	}

	// center window
	{
		auto shape = app_win_->shape();
		auto app_center = shape.client_position + (shape.client_size / 2);
		auto new_center = shape.client_position + (new_size / 2);
		auto diff = new_center - app_center;
		shape.client_position = app_win_->client_position() - diff;
		shape.client_size = new_size;
		app_win_->shape(shape);
	}
}

void tex_view::on_input(const input_t& inp)
{
	switch (inp.type)
	{
		case input_type::menu:   on_menu(inp);   break;
		case input_type::hotkey: on_hotkey(inp); break;
	}
}

void tex_view::on_menu(const input_t& inp)
{
	switch (inp.menu.id)
	{
		case ui::menu_item::file_open: open_file_dialog(); break;
		case ui::menu_item::file_exit: running_ = false; break;
		case ui::menu_item::view_zoom_quarter:
		case ui::menu_item::view_zoom_half:
		case ui::menu_item::view_zoom_original:
		case ui::menu_item::view_zoom_double: on_zoom(inp.menu.id); break;
		case ui::menu_item::help_about: about_->show_modal(app_win_); break;
		default: erh_.on_input(inp); break;
	}
}

void tex_view::on_hotkey(const input_t& inp)
{
	switch (inp.hotkey.id)
	{
		case ui::hotkey::file_open: open_file_dialog(); break;
		case ui::hotkey::view_zoom_quarter:
		case ui::hotkey::view_zoom_half:
		case ui::hotkey::view_zoom_original:
		case ui::hotkey::view_zoom_double: on_zoom(inp.hotkey.id - ui::hotkey::view_zoom_quarter + ui::menu_item::view_zoom_quarter); break; // @tony: need a better way of tying a hotkey to a menu entry
	}
}

void tex_view::refresh()
{
	if (!gpu_win_->is_window_thread())
		throw std::invalid_argument("must execute on main thread");

	auto cl = dev_->immediate();
	cl->set_rtv(dev_->get_presentation_rtv());
	cl->set_srvs(0, 1, &texture_);
	cl->draw(3);
	dev_->present();
}

void tex_view::ui_run()
{
	core_thread_traits::begin_thread("UI Thread");

	try
	{
		// create the UI window
		{
			{
				window::init_t i;
				i.title = s_app_name;
				i.icon = (icon_handle)load_icon(IDI_APPICON);
				i.alt_f4_closes = true;
				i.on_event = std::bind(&tex_view::on_event_app, this, std::placeholders::_1);
				i.on_input = std::bind(&tex_view::on_input, this, std::placeholders::_1);
				i.shape.state = window_state::hidden;
				i.shape.style = window_style::sizable_with_menu_and_statusbar;
				i.shape.client_size = int2(256, 256);
				app_win_ = window::make(i);
				int hk_id = app_win_->new_hotkeys(ui::s_hotkeys);
				app_win_->set_hotkeys(hk_id);
			}

			// create the about box
			{
				oDECLARE_ABOUT_INFO(i, load_icon(IDI_APPICON));
				about_ = about::make(i);
			}

			// attach gpu window to gui window
			gpu_win_->parent(app_win_);
			gpu_win_->show();
		}

		running_ = true;
		app_win_->show();

		while (running_)
			app_win_->flush_messages();

		gpu_win_->parent(nullptr);
		app_win_ = nullptr;
	}

	catch (std::exception& e)
	{
		msgbox(msg_type::info, nullptr, s_app_name, "ERROR\n%s", e.what());
	}

	core_thread_traits::end_thread();
}

void tex_view::run()
{
	try
	{
		running_ = true;
		while (running_)
		{
			gpu_win_->flush_messages();

			if (new_image_ready_)
			{
				texture_ = dev_->new_texture("image", new_image_);
				new_image_.deinitialize();
				new_image_ready_ = false;
			
				refresh();
			}
		}
	}
	catch (std::exception& e)
	{
		msgbox(msg_type::info, nullptr, "oTexApp Main Thread", "ERROR\n%s", e.what());
	}
}

int main(int argc, const char* argv[])
{
	reporting::set_prompter(prompt_msgbox);
	tex_view app;
	app.run();
	return 0;
}
