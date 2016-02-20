// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/progress_bar.h>
#include <oGUI/Windows/oWinRect.h>
#include <oGUI/Windows/oWinWindowing.h>

namespace ouro {

class progress_bar_impl : public progress_bar
{
public:
	progress_bar_impl(const char* _Title, ouro::icon_handle _hIcon
		, const std::function<void()>& _OnStop);


	// basic_window API
	ouro::window_handle native_handle() const override { return Window->native_handle(); }
	display::id display_id() const override { return Window->display_id(); }
	bool is_window_thread() const override { return Window->is_window_thread(); }
	void flush_messages(bool wait_for_next = false) override { Window->flush_messages(wait_for_next); }
	void quit() override { Window->quit(); }
	void debug(bool _Debug) override { Window->debug(_Debug); }
	bool debug() const override { return Window->debug(); }
	void state(window_state _State) override { Window->state(_State); }
	window_state state() const override { return Window->state(); }
	void client_position(const int2& _ClientPosition) override { Window->client_position(_ClientPosition); }
	int2 client_position() const override { return Window->client_position(); }
	int2 client_size() const override { return Window->client_size(); }
	void icon(ouro::icon_handle _hIcon) override { Window->icon(_hIcon); }
	ouro::icon_handle icon() const override { return Window->icon(); }
	void user_cursor(ouro::cursor_handle _hCursor) override { Window->user_cursor(_hCursor); }
	ouro::cursor_handle user_cursor() const override { return Window->user_cursor(); }
	void client_cursor(const mouse_cursor& c) override { Window->client_cursor(c); }
	mouse_cursor client_cursor() const override { return Window->client_cursor(); }
	void set_titlev(const char* _Format, va_list _Args) override { Window->set_titlev(_Format, _Args); }
	char* get_title(char* _StrDestination, size_t _SizeofStrDestination) const override { return Window->get_title(_StrDestination, _SizeofStrDestination); }
	void parent(const std::shared_ptr<basic_window>& _Parent) override { Window->parent(_Parent); }
	std::shared_ptr<basic_window> parent() const override { return Window->parent(); }
	void owner(const std::shared_ptr<basic_window>& _Owner) override { Window->owner(_Owner); }
	std::shared_ptr<basic_window> owner() const override { return Window->owner(); }
	void sort_order(window_sort_order _SortOrder) override { Window->sort_order(_SortOrder); }
	window_sort_order sort_order() const override { return Window->sort_order(); }
	void focus(bool _Focus) override { Window->focus(_Focus); }
	bool has_focus() const override { return Window->has_focus(); }

	// progress_bar API
	void stop_button(bool _Show) override { Window->dispatch([=] { oWinControlSetVisible(get(PB_BUTTON), _Show); }); }
	bool stop_button() const override { return oWinControlIsVisible(get(PB_BUTTON)); }
	void stopped(bool _Stopped = true) override;
	bool stopped() const override { return oWinControlGetErrorState(get(PB_PROGRESS)); }
	void set_textv(const char* _Format, va_list _Args) override;
	char* get_text(char* _StrDestination, size_t _SizeofStrDestination) const override { return oWinControlGetText(_StrDestination, _SizeofStrDestination, get(PB_TEXT)); }
	void set_subtextv(const char* _Format, va_list _Args) override;
	char* get_subtext(char* _StrDestination, size_t _SizeofStrDestination) const override { return oWinControlGetText(_StrDestination, _SizeofStrDestination, get(PB_SUBTEXT)); }
	void set_percentage(int _Percentage) override { Window->dispatch([=] { set_percentage_internal(_Percentage); }); }
	void add_percentage(int _Percentage) override { Window->dispatch([=] { set_percentage_internal(percentage() + _Percentage); }); }
	int percentage() const override { return oWinControlGetRangePosition(get(PB_PROGRESS)); }
	
private:
	std::shared_ptr<window> Window;
	std::function<void()> OnStop;

	enum PB_CONTROL
	{
		PB_TEXT,
		PB_SUBTEXT,
		PB_BUTTON,
		PB_PERCENT,
		PB_MARQUEE,
		PB_PROGRESS,
		PB_CONTROL_COUNT,
	};

	std::array<ouro::window_handle, PB_CONTROL_COUNT> Controls;
	int Percentage;

private:
	void on_event(const window::basic_event& _Event);
	void on_input(const ouro::input_t& _Input);
	void make_controls(const window::create_event& _CreateEvent);
	HWND get(PB_CONTROL _Control) const { return (HWND)this->Controls[_Control]; }
	void set_percentage_internal(HWND _hProgress, HWND _hMarquee, HWND _hPercent, int _Percentage);
	void set_percentage_internal(int _Percentage) { set_percentage_internal(get(PB_PROGRESS), get(PB_MARQUEE), get(PB_PERCENT), _Percentage); }
};

progress_bar_impl::progress_bar_impl(const char* _Title, ouro::icon_handle _hIcon, const std::function<void()>& _OnStop)
	: OnStop(_OnStop)
	, Percentage(-1)
{
	Controls.fill(nullptr);
	window::init_t i;
	i.title = _Title;
	i.icon = _hIcon;
	i.on_input = std::bind(&progress_bar_impl::on_input, this, std::placeholders::_1);
	i.on_event = std::bind(&progress_bar_impl::on_event, this, std::placeholders::_1);
	i.shape.client_size = int2(320, 106);
	i.shape.state = window_state::hidden;
	i.shape.style = window_style::fixed;
	Window = window::make(i);
	set_percentage_internal(-1);
}

std::shared_ptr<progress_bar> progress_bar::make(const char* _Title, ouro::icon_handle _hIcon, const std::function<void()>& _OnStop)
{
	return std::make_shared<progress_bar_impl>(_Title, _hIcon, _OnStop);
}

void progress_bar_impl::make_controls(const window::create_event& _CreateEvent)
{
	const int2 ProgressBarSize(270, 22);
	const int2 ButtonSize(75, 23);

	const int2 Inset(10, 10);
	const auto rParent = int4(int2(0, 0), _CreateEvent.shape.client_size);

	ouro::control_info Descs[PB_CONTROL_COUNT];

	// progress/marquee bars
	{
		auto rChild = int4(int2(Inset.x, 0), ProgressBarSize);
		auto rText = ouro::resolve_rect(rParent, rChild, alignment::middle_left, true);
		Descs[PB_MARQUEE].type = control_type::progressbar_unknown;
		Descs[PB_MARQUEE].position = oWinRectPosition(oWinRect(rText));
		Descs[PB_MARQUEE].size = ProgressBarSize;

		Descs[PB_PROGRESS].type = control_type::progressbar;
		Descs[PB_PROGRESS].position = oWinRectPosition(oWinRect(rText));
		Descs[PB_PROGRESS].size = ProgressBarSize;
	}

	// percentage text
	{
		const auto& cpb = Descs[PB_PROGRESS];
		Descs[PB_PERCENT].type = control_type::label;
		Descs[PB_PERCENT].text = "0%";
		Descs[PB_PERCENT].position = int2(cpb.position.x + cpb.size.x + 10, cpb.position.y + 3);
		Descs[PB_PERCENT].size = int2(35, cpb.size.y);
	}

	// Stop button
	{
		auto rChild = int4(-Inset, ButtonSize);
		auto rButton = ouro::resolve_rect(rParent, rChild, alignment::bottom_right, true);

		Descs[PB_BUTTON].type = control_type::button;
		Descs[PB_BUTTON].text = "&Stop";
		Descs[PB_BUTTON].position = oWinRectPosition(oWinRect(rButton));
		Descs[PB_BUTTON].size = oWinRectSize(oWinRect(rButton));
	}

	// text/subtext
	{
		Descs[PB_TEXT].type = control_type::label_centered;
		Descs[PB_TEXT].position = Inset;
		Descs[PB_TEXT].size = int2(_CreateEvent.shape.client_size.x - 2*Inset.x, 20);

		Descs[PB_SUBTEXT].type = control_type::label;
		Descs[PB_SUBTEXT].position = int2(Inset.x, Descs[PB_PROGRESS].position.y + Descs[PB_PROGRESS].size.y + 5);
		Descs[PB_SUBTEXT].size = int2((Descs[PB_PROGRESS].size.x * 3) / 4, 20);
	}

	for (short i = 0; i < PB_CONTROL_COUNT; i++)
	{
		Descs[i].parent = _CreateEvent.window;
		Descs[i].id = i;
		Controls[i] = (ouro::window_handle)oWinControlCreate(Descs[i]);
	}
}

void progress_bar_impl::on_event(const window::basic_event& _Event)
{
	switch (_Event.type)
	{
		case event_type::creating:
		{
			make_controls(_Event.as_create());
			break;
		}

		default:
			break;
	}
}

void progress_bar_impl::on_input(const ouro::input_t& input)
{
	if (input.type == ouro::input_type::control && input.control.status == control_status::activated && input.control.id == PB_BUTTON && OnStop)
	{
		stopped(true);
		OnStop();
	}
}

void progress_bar_impl::stopped(bool _Stopped)
{
	Window->dispatch([=]
	{
		oWinControlSetErrorState(get(PB_PROGRESS), _Stopped);
		oWinControlSetErrorState(get(PB_MARQUEE), _Stopped);
	});
}

void progress_bar_impl::set_textv(const char* _Format, va_list _Args)
{
	lstring s;
	if (-1 == vsnprintf(s, _Format, _Args))
	{
		ellipsize(s);
		oThrow(std::errc::no_buffer_space, "");
	}
	Window->dispatch([=] { oWinControlSetText(get(PB_TEXT), s.c_str()); });
}

void progress_bar_impl::set_subtextv(const char* _Format, va_list _Args)
{
	lstring s;
	if (-1 == vsnprintf(s, _Format, _Args))
	{
		ellipsize(s);
		oThrow(std::errc::no_buffer_space, "");
	}
	Window->dispatch([=] { oWinControlSetText(get(PB_SUBTEXT), s.c_str()); });
}

static void ensure_visible(HWND _hControl)
{
	if (!oWinControlIsVisible(_hControl))
		oWinControlSetVisible(_hControl);
}

static void ensure_hidden(HWND _hControl)
{
	if (oWinControlIsVisible(_hControl))
		oWinControlSetVisible(_hControl, false);
}

void progress_bar_impl::set_percentage_internal(HWND _hProgress, HWND _hMarquee, HWND _hPercent, int _Percentage)
{
	if (_Percentage < 0)
	{
		ensure_hidden(_hProgress);
		ensure_hidden(_hPercent);
		ensure_visible(_hMarquee);
	}

	else
	{
		UINT p = (UINT)__max(0, __min(100, (UINT)_Percentage));
		oWinControlSetRangePosition(_hProgress, p);

		char buf[16];
		snprintf(buf, "%u%%", p);
		ellipsize(buf);
		if (!oWinControlSetText(_hPercent, buf))
			throw std::runtime_error("oWinControlSetText failed");

		ensure_hidden(_hMarquee);
		ensure_visible(_hProgress);
		ensure_visible(_hPercent);
	}

	Percentage = _Percentage;
}

}
