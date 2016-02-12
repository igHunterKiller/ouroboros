// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oGPU/gpu.h>
#include <oGUI/window.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oSystem/display.h>
#include <oSystem/system.h>

using namespace ouro;
using namespace ouro::gpu;

static const bool kInteractiveMode = false;

class WindowInWindow
{
public:
	WindowInWindow()
		: Counter(0)
		, Running(true)
	{
		// Create the windows
		{
			window::init_t i;
			i.title = "Window-In-Window Test";
			i.on_event = std::bind(&WindowInWindow::ParentEventHook, this, std::placeholders::_1);
			i.shape.state = window_state::hidden;
			i.shape.style = window_style::sizable;
			i.shape.client_size = int2(640, 480);
			ParentWindow = window::make(i);
		}

		{
			window::init_t i;
			i.shape.state = window_state::restored;
			i.shape.style = window_style::borderless;
			i.shape.client_position = int2(20,20);
			i.shape.client_size = int2(600,480-65);
			i.on_event = std::bind(&WindowInWindow::GPUWindowEventHook, this, std::placeholders::_1);
			GPUWindow = window::make(i);
		}

		// Create the device
		{
			device_init i("TestDevice");
			i.api_version = version_t(10,0);
			i.enable_driver_reporting = true;
			dev_ = new_device(i, GPUWindow.get());
		}

		GPUWindow->parent(ParentWindow);
		ParentWindow->show();
	}

	inline bool running() const { return Running; }

	void render()
	{
		if (!dev_->get_presentation_rtv())
			return;

		auto rtv = dev_->get_presentation_rtv();
		dev_->immediate()->set_rtvs(1, &rtv);
		dev_->immediate()->clear_rtv(dev_->get_presentation_rtv(), (Counter & 0x1) ? color::white : color::blue);
		
		dev_->present();
	}

	void ParentEventHook(const window::basic_event& _Event)
	{
		switch (_Event.type)
		{
			case event_type::creating:
			{
				control_info ButtonDesc;
				ButtonDesc.parent = _Event.window;
				ButtonDesc.type = control_type::button;
				ButtonDesc.text = "Push Me";
				ButtonDesc.size = int2(100,25);
				ButtonDesc.position = int2(10,480-10-ButtonDesc.size.y);
				ButtonDesc.id = 0;
				ButtonDesc.starts_new_group = false;
				hButton = oWinControlCreate(ButtonDesc);
				break;
			}

			case event_type::closing:
				Running = false;
				break;

			case event_type::sizing:
			{
				if (dev_)
					dev_->on_window_resizing();
				break;
			}

			case event_type::sized:
			{
				if (GPUWindow)
					GPUWindow->client_size(_Event.as_shape().shape.client_size - int2(40,65));

				if (dev_)
					dev_->on_window_resized();

				SetWindowPos(hButton, 0, 10, _Event.as_shape().shape.client_size.y-10-25, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
				break;
			}

			default:
				break;
		}
	}

	void GPUWindowEventHook(const window::basic_event& _Event)
	{
	}

	window* get_window() { return ParentWindow.get(); }

	surface::image snapshot_and_wait()
	{
		future<surface::image> snapshot = ParentWindow->snapshot();
		while (!snapshot.is_ready()) { flush_messages(); }
		return snapshot.get();
	}

	void flush_messages()
	{
		GPUWindow->flush_messages();
		ParentWindow->flush_messages();
	}

	void increment_clear_counter() { Counter++; }

private:
	ref<device> dev_;
	std::shared_ptr<window> ParentWindow;
	std::shared_ptr<window> GPUWindow;

	HWND hButton;
	int Counter;
	bool Running;
};

oTEST(oGPU_window_in_window)
{
	if (srv.is_remote_session())
	{
		srv.trace("Detected remote session: differing text anti-aliasing will cause bad image compares");
		throw std::system_error(std::errc::not_supported, std::system_category(), "Detected remote session: differing text anti-aliasing will cause bad image compares");
	}

	// Turn display power on, otherwise the test will fail
	display::set_power_on();

	bool success = false;
	WindowInWindow test;

	if (kInteractiveMode)
	{
		while (test.running())
		{
			test.flush_messages();

			std::this_thread::sleep_for(std::chrono::seconds(1));
			test.increment_clear_counter();

			test.render();
		}
	}

	else
	{
		test.flush_messages();
		test.render();
		surface::image snapshot = test.snapshot_and_wait();
		srv.check(snapshot);
		test.increment_clear_counter();
		test.flush_messages();
		test.render();
		snapshot = test.snapshot_and_wait();
		srv.check(snapshot, 1);
	}
}
