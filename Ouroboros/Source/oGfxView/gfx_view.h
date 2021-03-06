// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oCore/ref.h>

#include <oMath/pov.h>

#include <oGUI/menu.h>
#include <oGUI/enum_radio_handler.h>
#include <oGUI/window.h>

#include <oSystem/peripherals.h>
#include "../about_ouroboros.h"

#include <oGfx/renderer.h>
#include <oGfx/scene.h>

#include <oMath/gizmo.h>

#include "camera_control.h"
#include "menu.h"
#include "hotkeys.h"

namespace ouro {

class gfx_view
{
public:
	gfx_view();
	~gfx_view();

	virtual void initialize();
	virtual void deinitialize();
	virtual void update(float delta_time) {}
	virtual void submit_scene(gfx::renderer_t& renderer) {}
	virtual void on_view_default() {}
	virtual void on_resized(uint32_t new_width, uint32_t new_height) {}

	virtual void request_model_load(gfx::model_registry& registry, const uri_t& uri_ref) {}

	void run();

protected:

	// adjust the camera to a position looking at a specific target
	void focus(const float3& eye, const float3& at) { camera_control_.focus(eye, at); }

	// loads a 2d texture for rendering
	gfx::texture2d_t load2d(const uri_t& uri_ref) { return renderer_.get_texture2d_registry()->load(uri_ref, nullptr); }
	gfx::model_t     load_mesh(const uri_t& uri_ref) { return renderer_.get_model_registry()->load(uri_ref, nullptr); }

	const keyboard_t& keyboard() const { return kb_; }
	const mouse_t& mouse() const { return mouse_; }

private:
// _____________________________________________________________________________
// Basics

	std::shared_ptr<window> gpu_win_;
	std::shared_ptr<window> app_win_;
	std::shared_ptr<about> about_;
	bool running_;

// _____________________________________________________________________________
// UI and functionality/input-related objects

	std::thread ui_thread_;
	std::array<menu_handle, ui::menu::count> menus_;
	gui::menu::enum_radio_handler erh_;
	window_state pre_fullscreen_state_;
	bool standalone_mode_;
	bool allow_standalone_mode_change_;
	
	mouse_t mouse_;
	keyboard_t kb_;

	camera_control camera_control_;
	gizmo gizmo_;
	gizmo::tessellation_info_t gizmo_tess_;

	// handlers: events should be handled separately but input  should funnel to the same handler.
	void on_event_app(const window::basic_event& evt);
	void on_event_gpu(const window::basic_event& evt);
	void on_input(const input_t& inp);
	void on_menu(const input_t& inp);
	void on_hotkey(const input_t& inp);
	void on_keypress(const input_t& inp);

	// UI elements: these don't showcase the gpu api per-sae, but
	// allow its features to be shown integrated with OS UI drawing
	void create_menus(const window::create_event& evt);
	void enable_status_bar_styles(bool enabled);
	void set_manip_space(const gizmo::space_t& space);
	void check_state(window_state state);
	void check_style(window_style style);

	void load_mesh_dialog();

	// GPU window inside UI window v. GPU window standalone
	// window::shape() must be called from the window's thread, so when in standalone
	// mode it is necessary to cache the gpu thread's shape before redirecting the 
	// operation to the UI thread
	void set_standalone_mode_internal(bool enabled, const window_shape& gpu_shape = window_shape());
	void set_standalone_mode(bool enabled);
	void toggle_fullscreen_coop(window* win);

	// returns true if pov was updated this frame
	bool update_pov();
	void ui_run();

// _____________________________________________________________________________
// Rendering objects and data

	gfx::renderer_t renderer_;
	pov_t pov_;

	// Pick ray visualization, maintains position for inspection from different angles
	float3 mouse_seg0;
	float3 mouse_seg1;

	void submit_pick_ray();
	void submit_gizmo();
	void submit_world_axis();

protected:
// _____________________________________________________________________________
// Experimental - not baked enough to put anywhere yet.

	gfx::scene_t scene_;
	gfx::pivot_t* current_selected_;

	// Selects objects from the scene that intersect the current pick ray
	void update_scene_selection();

	void display_mouse_position(const float3& pos)
	{
		// @tony: fixme
		//app_win_->set_status_text(ui::statusbar::mouse_coords, "%.1f %.1f %.1f", pos.x, pos.y, pos.z);
		app_win_->set_status_text(ui::statusbar::camera_coords, "%.1f %.1f %.1f", pos.x, pos.y, pos.z);
	}
};

}
