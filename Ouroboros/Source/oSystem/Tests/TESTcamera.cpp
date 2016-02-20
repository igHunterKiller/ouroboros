// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSystem/camera.h>
#include <oCore/assert.h>
#include <oCore/stringize.h>

using namespace ouro;

oTEST(oSystem_camera)
{
	bool reported = false;

	camera::enumerate([&](std::shared_ptr<camera> _Camera)->bool
	{
		if (!reported)
		{
			srv.status("camera \"%s\" detected. See trace for more info.", _Camera->name());
			reported = true;
		}

		oTrace("oCore::camera \"%s\", supports:", _Camera->name());
		_Camera->enumerate_modes([&](const camera::mode& _Mode)->bool
		{
			oTrace("- %dx%d %s %d bitrate", _Mode.dimensions.x, _Mode.dimensions.y, as_string(_Mode.format), _Mode.bit_rate);
			return true;
		});
		return true;
	});

	if (!reported)
		srv.status("no camera detected");

#if 0 // @tony: make a simple test app for cameras

	struct CONTEXT
	{
		CONTEXT(ref<threadsafe oCamera> _pCamera)
			: Camera(_pCamera)
			, LastFrame(ouro::invalid)
			, Running(true)
		{}

		ref<threadsafe oCamera> Camera;
		ref<oWindow> Window;
		unsigned int LastFrame;
		bool Running;

		void OnEvent(const window::basic_event& _Event)
		{
			switch (_Event.Type)
			{
				case event_type::closing:
					Running = false;
					break;
				default:
					break;
			}
		}
	};

	std::vector<CONTEXT> Contexts;
	unsigned int index = 0;

	while (1)
	{
		ref<threadsafe oCamera> Camera;
		if (oCameraEnum(index++, &Camera))
		{
			oCamera::MODE mode;
			mode.Dimensions = int2(640, 480);
			mode.Format = surface::r8g8b8_unorm;
			mode.BitRate = invalid;

			oCamera::MODE closest;
			if (!Camera->FindClosestMatchingMode(mode, &closest))
				oAssert(false, "");

			if (!Camera->SetMode(closest))
			{
				oMSGBOX_DESC d;
				d.Type = oMSGBOX_ERR;
				d.Title = "oCamera Test";
				oMsgBox(d, "Camera %s does not support mode %s %dx%d", Camera->GetName(), as_string(mode.Format), mode.Dimensions.x, mode.Dimensions.y);
				continue;
			}

			Contexts.push_back(Camera);
		}

		// this should've been replaced by exception handling a while ago
		else if (oEr ro r G e tLa st() == std::errc::no_such_device)
			break;
	}

//	if (oErrorGetLast() == std::errc::no_such_device && !Contexts.empty())
//	{
//		oMSGBOX_DESC d;
//		d.Type = oMSGBOX_ERR;
//		d.Title = "oCamera Test";
//#if o64BIT
//		oMsgBox(d, "Enumerating system cameras is not supported on 64-bit systems.");
//#else
//		oMsgBox(d, "Failed to enumerate system cameras because an internal interface could not be found.");
//#endif
//		return -1;
//	}

	if (Contexts.empty())
	{
		oMSGBOX_DESC d;
		d.Type = oMSGBOX_INFO;
		d.Title = "oCamera Test";
		oMsgBox(d, "No cameras were found, so no windows will open");
	}

	for (size_t i = 0; i < Contexts.size(); i++)
	{
		oCamera::DESC cd;
		Contexts[i].Camera->GetDesc(&cd);

		lstring Title;
		snprintf(Title, "%s (%dx%d %s)", Contexts[i].Camera->GetName(), cd.Mode.Dimensions.x, cd.Mode.Dimensions.y, as_string(cd.Mode.Format));

		window::init_t init;
		init.shape.ClientSize = cd.Mode.Dimensions;
		init.shape.ClientPosition = int2(30, 30) * int2(oUInt(i + 1), oUInt(i + 1));
		init.title = Title;
		init.on_event = std::bind(&CONTEXT::OnEvent, &Contexts[i], std::placeholders::_1);
		Contexts[i].Window = window::make(init);
	}

	int OpenWindowCount = 0;
	for (size_t i = 0; i < Contexts.size(); i++)
		if (Contexts[i].Window)
			OpenWindowCount++;

	while (OpenWindowCount)
	{
		for (size_t i = 0; i < Contexts.size(); i++)
		{
			Contexts[i].Window->FlushMessages();

			oCamera::DESC cd;
			Contexts[i].Camera->GetDesc(&cd);

			oCamera::MAPPED mapped;
			if (Contexts[i].Camera->Map(&mapped))
			{
				if (mapped.Frame != Contexts[i].LastFrame)
				{
					HWND hWnd = (HWND)Contexts[i].Window->GetNativeHandle();

					oVERIFY(oGDIStretchBits(hWnd, cd.Mode.Dimensions, cd.Mode.Format, mapped.pData, mapped.RowPitch));
					Contexts[i].LastFrame = mapped.Frame;

					float fps = Contexts[i].Camera->GetFPS();
					sstring sFPS;
					snprintf(sFPS, "FPS: %.01f", fps + (rand() %100) / 100.0f);
					
					RECT rClient;
					GetClientRect(hWnd, &rClient);

					text_info td;
					td.Foreground = CadetBlue;
					td.Position = int2(0, 0);
					td.Size = oWinRectSize(rClient);
					td.Shadow = Black;
					td.ShadowOffset = int2(1,1);
					td.Alignment = alignment::middle_left;

					scoped_getdc hDC(hWnd);
					draw_text(hDC, td, sFPS);
				}

				Contexts[i].Camera->Unmap();
			}
		}

		OpenWindowCount = 0;
		for (size_t i = 0; i < Contexts.size(); i++)
			if (Contexts[i].Running)
				OpenWindowCount++;
	}

	return 0;
#endif
}
