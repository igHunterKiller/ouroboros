// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "gfx_view.h"
#include "resource.h"

#include <oCore/timer.h>
#include <oCore/color.h>

#include <oMath/primitive.h>

#include <oSystem/filesystem.h>
#include <oSystem/reporting.h>
#include <oSystem/thread_traits.h>

#include <oGUI/msgbox.h>
#include <oGUI/Windows/win_gdi_bitmap.h>

#include <oConcurrency/backoff.h>

using namespace ouro;
using namespace ouro::gui;
using namespace windows::gdi;

static const char* s_app_name = "oGfxView";

#include "menu.h"
#include "hotkeys.h"

ouro::gfx::render_settings_t g_render_settings;

gfx_view::gfx_view()
	: running_(false)
	, standalone_mode_(false)
	, allow_standalone_mode_change_(true)
	, current_selected_(nullptr)
	, mouse_seg0(-10000.0f, -10000.0f, -10000.0f)
	, mouse_seg1(-10000.0f, -10000.0f, -10000.0f)
{
	pov_ = pov_t(uint2(256,256), oRECOMMENDED_PC_FOVX_RADIANSf, 0.01f, 1000.0f);

	// create the render display window
	{
		window::init_t i;
		i.title = s_app_name;
		i.icon = (icon_handle)load_icon(IDI_APPICON);
		i.alt_f4_closes = true;
		i.on_event = std::bind(&gfx_view::on_event_gpu, this, std::placeholders::_1);
		i.on_input = std::bind(&gfx_view::on_input, this, std::placeholders::_1);
		i.keyboard = &kb_;
		i.mouse = &mouse_;
		i.shape.state = window_state::hidden;
		i.shape.style = window_style::borderless;
		i.shape.client_size = int2(256, 256);
		i.shape.client_position = int2(0, 0);
		gpu_win_ = window::make(i);
		int hotkeys_id = gpu_win_->new_hotkeys(ui::s_hotkeys);
		gpu_win_->set_hotkeys(hotkeys_id);
	}
	
	{
		gfx::scene_init_t si;
		si.max_pivots = 100;
		scene_.initialize(si);
	}

	{
		gfx::renderer_init_t i;
		renderer_.initialize(i, gpu_win_.get());
	}

	// initialize camera controls
	camera_control_.focus(float3(0.0f, 0.0f, 15.0f), kZero3);
	gizmo_.space(gizmo::space::local);

	// start the ui thread
	running_ = false;
	ui_thread_ = std::thread(std::bind(&gfx_view::ui_run, this));

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

gfx_view::~gfx_view()
{
	renderer_.deinitialize();

	scene_.deinitialize();

	ui_thread_.join();
}

void gfx_view::create_menus(const window::create_event& evt)
{
	for (auto& m : menus_)
		m = menu::make_menu();

	for (const auto& h : ui::s_menu_hierarchy)
		menu::append_submenu(h.parent == ui::menu::top_level ? evt.menu : menus_[h.parent], menus_[h.child], h.name);

	// File menu
	menu::append_item(menus_[ui::menu::file], ui::menu_item::file_exit, "E&xit\tAlt+F4");

	// Edit menu
	// (nothing yet)
	menu::append_enum_items(gizmo::space::count, menus_[ui::menu::edit_manip_space], ui::menu_item::edit_manip_space_first, ui::menu_item::edit_manip_space_last, 0);
	erh_.add               (                     menus_[ui::menu::edit_manip_space], ui::menu_item::edit_manip_space_first, ui::menu_item::edit_manip_space_last, [=](int manip_space) { set_manip_space((gizmo::space_t)manip_space); });

	// View menu
	menu::append_enum_items(camera_control::type::count, menus_[ui::menu::view_camera_control], ui::menu_item::view_camera_control_first, ui::menu_item::view_camera_control_last, (int)camera_control::type::dcc);
	erh_.add               (                             menus_[ui::menu::view_camera_control], ui::menu_item::view_camera_control_first, ui::menu_item::view_camera_control_last, [=](int camera_control)
	{
		auto type = (camera_control::type_t)camera_control;
		oTrace("change to %s", as_string(type));
		app_win_->set_status_text(ui::statusbar::camera_type, as_string(type));
		camera_control_.type(type);
	});

	menu::append_enum_items(window_style::count, menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, evt.shape.style);
	enable_status_bar_styles(true);

	erh_.add         (menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, [=](int border_style) { app_win_->style((window_style)border_style); });
	menu::check_radio(menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, ui::menu_item::view_style_first + (int)window_style::sizable_with_menu);

	menu::append_enum_items(window_state::count, menus_[ui::menu::view_state], ui::menu_item::view_state_first, ui::menu_item::view_state_last, evt.shape.state);
	erh_.add               (                     menus_[ui::menu::view_state], ui::menu_item::view_state_first, ui::menu_item::view_state_last, [=](int state) { app_win_->show((window_state)state); });

	menu::append_enum_items(gfx::fullscreen_mode::count, menus_[ui::menu::view_render_state], ui::menu_item::view_render_state_first, ui::menu_item::view_render_state_last, (int)gfx::fullscreen_mode::normal);
	erh_.add               (                         menus_[ui::menu::view_render_state], ui::menu_item::view_render_state_first, ui::menu_item::view_render_state_last, [=](int state) { g_render_settings.mode = (gfx::fullscreen_mode)state; });

	menu::append_item(menus_[ui::menu::view], ui::menu_item::view_default, "&Default View");
	
	menu::append_item(menus_[ui::menu::view], ui::menu_item::view_exclusive, "Fullscreen E&xclusive");

	// Help menu
	menu::append_item(menus_[ui::menu::help], ui::menu_item::help_about, "About...");
}

void gfx_view::enable_status_bar_styles(bool enabled)
{
	// enable styles not allowed for render target windows
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::fixed_with_statusbar,            enabled);
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::fixed_with_menu_and_statusbar,   enabled);
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::sizable_with_statusbar,          enabled);
	menu::enable(menus_[ui::menu::view_style], ui::menu_item::view_style_first + (int)window_style::sizable_with_menu_and_statusbar, enabled);
}

void gfx_view::set_manip_space(const gizmo::space_t& space)
{
	gizmo_.space(space);
}

void gfx_view::check_state(window_state state)
{
	menu::check_radio(menus_[ui::menu::view_state], ui::menu_item::view_state_first, ui::menu_item::view_state_last, ui::menu_item::view_state_first + (int)state);
}

void gfx_view::check_style(window_style style)
{
	menu::check_radio(menus_[ui::menu::view_style], ui::menu_item::view_style_first, ui::menu_item::view_style_last, ui::menu_item::view_style_first + (int)style);
}

void gfx_view::set_standalone_mode_internal(bool enabled, const window_shape& gpu_shape)
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

void gfx_view::set_standalone_mode(bool enabled)
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

void gfx_view::toggle_fullscreen_coop(window* win)
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

void gfx_view::on_event_gpu(const window::basic_event& evt)
{
	if (!gpu_win_)
		return;

	switch (evt.type)
	{
		case event_type::sizing:
		{
			renderer_.on_window_resizing();
			break;
		}

		case event_type::sized:
		{
			const auto& size = evt.as_shape().shape.client_size;
			renderer_.on_window_resized(size.x, size.y);
			pov_.viewport(size);
			on_resized(size.x, size.y);
			break;
		}

		case event_type::closing:
			running_ = false;
			break;
	}
}

void gfx_view::on_event_app(const window::basic_event& evt)
{
	switch (evt.type)
	{
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

		case event_type::deactivated:
		case event_type::lost_capture:
			gpu_win_->client_cursor(mouse_cursor::arrow);
			gpu_win_->capture(mouse_capture::none);
			camera_control_.reset();
			gizmo_.reset();
			mouse_.reset();
			kb_.reset();
			break;

		case event_type::closing:
			running_ = false;
			break;
	}
}

void gfx_view::on_input(const input_t& inp)
{
	// This handler is attached to both app and gpuwindows, 
	// so messages from either thread get pumped through here.

	switch (inp.type)
	{
		case input_type::menu:     on_menu    (inp); break;
		case input_type::hotkey:   on_hotkey  (inp); break;
	}
}

void gfx_view::on_menu(const input_t& inp)
{
	switch (inp.menu.id)
	{
		case ui::menu_item::file_exit:    running_ = false;             break;
		case ui::menu_item::help_about:   about_->show_modal(app_win_); break;
		case ui::menu_item::view_default: on_view_default();            break;
		default:                          erh_.on_input(inp);           break;
		case ui::menu_item::view_exclusive:
		{
			const bool checked = menu::checked(menus_[ui::menu::view], inp.menu.id);
			menu::check(menus_[ui::menu::view], inp.menu.id, !checked);
			app_win_->set_status_text(ui::statusbar::fullscreen_type, "Fullscreen %s", !checked ? "exclusive" : "cooperative");
			break;
		}
	}
}

void gfx_view::on_hotkey(const input_t& inp)
{
	switch (inp.hotkey.id)
	{
		case ui::hotkey::focus:
			if (current_selected_)
			{
				float4 bound = current_selected_->world_bound();
				bound.w *= 2.0f;
				camera_control_.focus(pov_.fov().y, bound);
			}

			break;

		case ui::hotkey::manip_scale:
			gizmo_.type(gizmo::type::scale);
			break;

		case ui::hotkey::manip_rotation:
			gizmo_.type(gizmo::type::rotate);
			break;

		case ui::hotkey::manip_translation:
			gizmo_.type(gizmo::type::translate);
			break;

		case ui::hotkey::manip_none:
			gizmo_.type(gizmo::type::none);
			break;

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
					auto dev = renderer_.dev();

					auto mode = dev->get_present_mode();
					auto new_mode = mode == gpu::present_mode::fullscreen_exclusive ? gpu::present_mode::windowed : gpu::present_mode::fullscreen_exclusive;
					dev->set_present_mode(new_mode);
					allow_standalone_mode_change_ = new_mode != gpu::present_mode::fullscreen_exclusive;
				}
				else
					toggle_fullscreen_coop(gpu_win_.get());
			}
			break;
		}
		case ui::hotkey::toggle_camera_control:
		{
			break;
		}
		case ui::hotkey::render_solid:
		case ui::hotkey::render_wire:
		case ui::hotkey::render_texcoord:
		case ui::hotkey::render_texcoordu:
		case ui::hotkey::render_texcoordv:
		{
			g_render_settings.mode = gfx::fullscreen_mode(inp.hotkey.id - ui::hotkey::render_solid);
			menu::check_radio(menus_[ui::menu::view_render_state], ui::menu_item::view_render_state_first, ui::menu_item::view_render_state_last, ui::menu_item::view_render_state_first + (int)g_render_settings.mode);
			break;
		}

		default:
			oTrace("Unhandled hotkey id = %d", inp.hotkey.id);
			break;
	}
}

void gfx_view::ui_run()
{
	core_thread_traits::begin_thread("UI Thread");

	try
	{
		// create the UI window
		{
			window::init_t i;
			i.title = s_app_name;
			i.icon = (icon_handle)load_icon(IDI_APPICON);
			i.alt_f4_closes = true;
			i.keyboard = &kb_;
			i.mouse = &mouse_;
			i.on_event = std::bind(&gfx_view::on_event_app, this, std::placeholders::_1);
			i.on_input = std::bind(&gfx_view::on_input, this, std::placeholders::_1);
			i.shape.state = window_state::hidden;
			i.shape.style = window_style::sizable_with_menu_and_statusbar;
			i.shape.client_size = int2(256, 256);
			app_win_ = window::make(i);

			// initialize some extra UI features
			{
				int hotkeys_id = app_win_->new_hotkeys(ui::s_hotkeys);
				app_win_->set_hotkeys(hotkeys_id);
			
				const int s_sections[] = { 240, 120, 120, 40, -1 };
				app_win_->set_num_status_sections(s_sections);
				app_win_->set_status_text(ui::statusbar::camera_coords, "X:%.1f Y:%.1f Z:%.1f P:%.1f Y:%.1f R:%.1f", 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
				app_win_->set_status_text(ui::statusbar::mouse_coords, "X:%.1f Y:%.1f Z:%.1f", 0.0f, 0.0f, 0.0f);
				app_win_->set_status_text(ui::statusbar::default_style_message, "F3 for default style");
				app_win_->set_status_text(ui::statusbar::camera_type, "Maya");
				app_win_->set_status_text(ui::statusbar::fullscreen_type, "Fullscreen cooperative");

				oDECLARE_ABOUT_INFO(inf, load_icon(IDI_APPICON));
				about_ = about::make(inf);
			}
		}

		running_ = true;
		while (running_)
			app_win_->flush_messages(true);
	}

	catch (std::exception& e)
	{
		msgbox(msg_type::info, nullptr, s_app_name, "ERROR\n%s", e.what());
	}

	core_thread_traits::end_thread();
}

bool gfx_view::update_pov()
{
	const bool alt_down = kb_.down(key::lalt) || kb_.down(key::ralt);
	
	camera_control_.enable(alt_down);
	
	bool updated = camera_control_.update(kb_, mouse_, *gpu_win_, pov_);
	
	// always updated the pointer depth if selection changes
	if (current_selected_)
	{
		const auto WSpos = current_selected_->position();
		const auto VSpos = pov_.world_to_view(WSpos);
		pov_.pointer_depth(VSpos.z);
	}

	else
		pov_.pointer_depth(0.0f);

	return updated;
}

void gfx_view::run()
{
	initialize();
	try
	{
		ouro::timer t;
		double the_time = t.seconds();

		while (running_)
		{
			float delta_time;
			{
				double now = t.seconds();
				double delta_time_d = now - the_time;
				the_time = now;
				delta_time = (float)delta_time_d;
			}

			gpu_win_->flush_messages();

			mouse_.update();
			kb_.update();

			// Update camera position & orientation
			bool pov_updated = update_pov();

			// If the user didn't change the camera, then gizmo is available
			bool gizmo_active = !pov_updated && mouse_.down(mouse_button::left);

			// Init an invalid pick so gizmo doesn't activate while rotating view
			float3 ws_pick0(FLT_MAX, FLT_MAX, FLT_MAX);
			float3 ws_pick1(FLT_MAX, FLT_MAX, FLT_MAX);

			// If not rotating view, use mouse coords as pick
			if (!pov_updated)
			{
				ws_pick0 = pov_.unproject_near(mouse_.x(), mouse_.y());
				ws_pick1 = pov_.unproject_far(mouse_.x(), mouse_.y());
			}

			// Update transform gizmo
			{
				gizmo_tess_ = gizmo_.update(pov_.viewport().dimensions, pov_.view_inverse(), ws_pick0, ws_pick1, gizmo_active);

				// Based on what happened, update mouse state or process selection of objects
				switch (gizmo_.state())
				{
					case gizmo::state::newly_active:   gpu_win_->capture(mouse_capture::absolute); break;
					case gizmo::state::newly_inactive: gpu_win_->capture(mouse_capture::none);     break;
					case gizmo::state::inactive:       update_scene_selection();                   break;
					default: break;
				}
			}

			update(delta_time);
			renderer_.begin_submit();
			renderer_.begin_view(&pov_, &g_render_settings);

			renderer_.submit(0, gfx::render_pass::initialize, gfx::render_technique::view_begin, nullptr);
			renderer_.submit(0, gfx::render_pass::resolve, gfx::render_technique::view_end, nullptr);

			// Do render-specific things. This will evolve towards some kind of scene traversal, but for now
			// each test app has to explicitly submit it's own scene.
			submit_scene(renderer_);

			renderer_.submit(0, gfx::render_pass::depth_resolve, gfx::render_technique::linearize_depth, nullptr);

			submit_pick_ray();
			submit_world_axis();
			submit_gizmo();

			renderer_.end_view(/*some resolve texture (display or offscreen)*/);
			renderer_.end_submit();
		}
	}
	catch (std::exception& e)
	{
		msgbox(msg_type::info, nullptr, s_app_name, "ERROR\n%s", e.what());
	}
	deinitialize();
}

void gfx_view::initialize()
{
}

void gfx_view::deinitialize()
{
}

void transform_pivot(const float4x4& tx, void* pivot)
{
	((gfx::pivot_t*)pivot)->world(tx);
}

void gfx_view::update_scene_selection()
{
	const bool alt_down = kb_.down(key::lalt) || kb_.down(key::ralt);
	if (!alt_down && mouse_.pressed(mouse_button::left))
	{
		int x = mouse_.x();
		int y = mouse_.y();

		const float3 pick0 = pov_.unproject_near(x, y);
		const float3 pick1 = pov_.unproject_far (x, y);

		gfx::pivot_t* pivots[10];
		size_t n = scene_.select_by_segment(pick0, pick1, pivots, countof(pivots));

		current_selected_ = n == 0 ? nullptr : pivots[0];

		if (current_selected_)
		{
			oTrace("Picked %u", current_selected_->uid());
			gizmo_.transform(current_selected_->world());
			gizmo_.set_transform_callback(transform_pivot, current_selected_);
		}

		else
		{
			oTrace("No pick");
		}
	}
}

void gfx_view::submit_pick_ray()
{
	bool update_pick_ray = mouse_.pressed(mouse_button::right);

	if (update_pick_ray)
	{
		int x = mouse_.x();
		int y = mouse_.y();

		mouse_seg0 = pov_.unproject_near(x, y);
		mouse_seg1 = pov_.unproject_far(x, y);
	}

	if (mouse_seg0.x > -10000.0f)
	{
		auto pick_sub    = (gfx::lines_submission_t*)renderer_.allocate(sizeof(gfx::lines_submission_t) + sizeof(gfx::render_line_t));
		auto pick_ray    = (gfx::render_line_t*)(pick_sub + sizeof(gfx::lines_submission_t));
			
		pick_ray->p0     = mouse_seg0;
		pick_ray->p1     = mouse_seg1;
		pick_ray->argb   = color::white;

		pick_sub->lines  = pick_ray;
		pick_sub->nlines = 1;

		renderer_.submit(0, gfx::render_pass::geometry, gfx::render_technique::draw_lines, pick_sub);
	}
}

void gfx_view::submit_world_axis()
{
	renderer_.submit(0, gfx::render_pass::debug, gfx::render_technique::draw_axis, nullptr);
}

void gfx_view::submit_gizmo()
{
	if (current_selected_)
	{
		auto* tess = renderer_.allocate<gizmo::tessellation_info_t>();
		*tess = gizmo_tess_;
		renderer_.submit(0, gfx::render_pass::debug, gfx::render_technique::draw_gizmo, tess);
	}
}

