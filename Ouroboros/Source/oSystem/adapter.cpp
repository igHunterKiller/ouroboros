// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/guid.h>
#include <oCore/ref.h>
#include <oSystem/adapter.h>
#include <oSystem/display.h>
#include <oSystem/windows/win_com.h>
#include <oSystem/windows/win_util.h>
#include <oMath/hlsl.h>
#include <regex>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <comutil.h>
#include <d3d11.h>
#include <dxgi.h>
#include <Wbemidl.h>

// Some GPU drivers have bugs in newer features that we use, so ensure we're at
// least on this version and hope there aren't regressions.

#define oNVVER_MAJOR 285
#define oNVVER_MINOR 62

#define oAMDVER_MAJOR 8
#define oAMDVER_MINOR 982

namespace ouro { namespace adapter { namespace detail {

// NVIDIA's version string is of the form "x.xx.xM.MMmm" where
// MMM is the major version and mm is the minor version.
// (outside function below so this doesn't get tracked as a leak)
static std::regex reNVVersionString("[0-9]+\\.[0-9]+\\.[0-9]+([0-9])\\.([0-9][0-9])([0-9]+)");

static version_t from_string_nv(const char* version_string)
{
	std::cmatch matches;
	oCheck(regex_match(version_string, matches, reNVVersionString), std::errc::invalid_argument, "not a well-formed NVIDIA version string: %s", oSAFESTRN(version_string));

	char major[4];
	major[0] = *matches[1].first;
	major[1] = *matches[2].first;
	major[2] = *(matches[2].first+1);
	major[3] = 0;
	return version_t(uint8_t(atoi(major)), uint8_t(atoi(matches[3].first)));
}

// AMD's version string is of the form M.mm.x.x where
// M is the major version and mm is the minor version.
// (outside function below so this doesn't get tracked as a leak)
static std::regex reAMDVersionString("([0-9]+)\\.([0-9]+)\\.[0-9]+\\.[0-9]+");

static version_t from_string_amd(const char* version_string)
{
	std::cmatch matches;
	oCheck(regex_match(version_string, matches, reAMDVersionString), std::errc::invalid_argument, "not a well-formed AMD version string: %S", oSAFESTRN(version_string));

	return version_t(static_cast<uint8_t>(atoi(matches[1].first)), static_cast<uint8_t>(atoi(matches[2].first)));
}

static version_t from_string_intel(const char* version_string)
{
	// This initial version was done based on a laptop which appears to have AMD-
	// like drivers for an Intel chip, so use the same parsing for now.
	return from_string_amd(version_string);
}

void enumerate_video_drivers(enumerate_fn enumerator, void* user)
{
	// redefine some MS GUIDs so we don't have to link to them

	// 4590F811-1D3A-11D0-891F-00AA004B2E24
	static const guid_t oGUID_CLSID_WbemLocator = { 0x4590F811, 0x1D3A, 0x11D0, { 0x89, 0x1F, 0x00, 0xAA, 0x00, 0x4B, 0x2E, 0x24 } };

	// DC12A687-737F-11CF-884D-00AA004B2E24
	static const guid_t oGUID_IID_WbemLocator = { 0xdc12a687, 0x737f, 0x11cf, { 0x88, 0x4D, 0x00, 0xAA, 0x00, 0x4B, 0x2E, 0x24 } };

	windows::com::ensure_initialized();

	ref<IWbemLocator> WbemLocator;
	oV(CoCreateInstance((const GUID&)oGUID_CLSID_WbemLocator
		, 0
		, CLSCTX_INPROC_SERVER
		, (const IID&)oGUID_IID_WbemLocator
		, (LPVOID*)&WbemLocator));

	ref<IWbemServices> WbemServices;
	oV(WbemLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2")
		, nullptr
		, nullptr
		, 0
		, 0
		, 0
		, 0
		, &WbemServices));

	oV(CoSetProxyBlanket(WbemServices
		, RPC_C_AUTHN_WINNT
		, RPC_C_AUTHZ_NONE
		, nullptr
		, RPC_C_AUTHN_LEVEL_CALL
		, RPC_C_IMP_LEVEL_IMPERSONATE
		, nullptr
		, EOAC_NONE));
	
	ref<IEnumWbemClassObject> Enumerator;
	oV(WbemServices->ExecQuery(bstr_t("WQL")
		, bstr_t("SELECT * FROM Win32_VideoController")
		, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY
		, nullptr, &Enumerator));

	ref<IWbemClassObject> WbemClassObject;
	while (Enumerator)
	{
		info_t info;

		ULONG uReturn = 0;
		WbemClassObject = nullptr;
		Enumerator->Next(WBEM_INFINITE, 1, &WbemClassObject, &uReturn);
		if (0 == uReturn)
			break;

		VARIANT vtProp;
		oV(WbemClassObject->Get(L"Description", 0, &vtProp, 0, 0));
		info.description = vtProp.bstrVal;
		VariantClear(&vtProp);
		oV(WbemClassObject->Get(L"PNPDeviceID", 0, &vtProp, 0, 0));
		info.plugnplay_id = vtProp.bstrVal;
		VariantClear(&vtProp);
		oV(WbemClassObject->Get(L"DriverVersion", 0, &vtProp, 0, 0));

		sstring StrVersion = vtProp.bstrVal;
		if (strstr(info.description, "NVIDIA"))
		{
			info.vendor = vendor::nvidia;
			info.version = from_string_nv(StrVersion);
		}
		
		else if (strstr(info.description, "ATI") || strstr(info.description, "AMD"))
		{
			info.vendor = vendor::amd;
			info.version = from_string_amd(StrVersion);
		}

		else if (strstr(info.description, "Intel"))
		{
			info.vendor = vendor::intel;
			info.version = from_string_intel(StrVersion);
		}

		else
		{
			info.vendor = vendor::unknown;
			info.version = version_t();
		}

		if (!enumerator(info, user))
			break;
	}
}

		} // namespace detail

static info_t get_info(int adapter_index, IDXGIAdapter* adapter)
{
	struct ctx_t
	{
		DXGI_ADAPTER_DESC adapter_desc;
		info_t adapter_info;
	};

	ctx_t ctx;
	adapter->GetDesc(&ctx.adapter_desc);
	
	// There's a new adapter called teh Basic Render Driver that is not a video driver, so 
	// it won't show up with enumerate_video_drivers, so handle it explicitly here.
	if (ctx.adapter_desc.VendorId == 0x1414 && ctx.adapter_desc.DeviceId == 0x8c) // Microsoft Basic Render Driver
	{
		ctx.adapter_info.description = "Microsoft Basic Render Driver";
		ctx.adapter_info.plugnplay_id = "Microsoft Basic Render Driver";
		ctx.adapter_info.version = version_t(1,0);
		ctx.adapter_info.vendor = vendor::microsoft;
		ctx.adapter_info.feature_level = version_t(11,1);
	}

	else
	{
		detail::enumerate_video_drivers([](const info_t& info, void* user)->bool
		{
			auto& ctx = *(ctx_t*)user;

			char vendor[128];
			char device[128];
			snprintf(vendor, "VEN_%X", ctx.adapter_desc.VendorId);
			snprintf(device, "DEV_%04X", ctx.adapter_desc.DeviceId);
			if (strstr(info.plugnplay_id, vendor) && strstr(info.plugnplay_id, device))
			{
				ctx.adapter_info = info;
				return false;
			}
			return true;
		}, &ctx);
	}

	*(int*)&ctx.adapter_info.id = adapter_index;

	D3D_FEATURE_LEVEL FeatureLevel;
	// Note that the out-device is null, thus this isn't that expensive a call
	if (SUCCEEDED(D3D11CreateDevice(
		adapter
		, D3D_DRIVER_TYPE_UNKNOWN
		, nullptr
		, 0 // D3D11_CREATE_DEVICE_DEBUG // squelches a _com_error warning
		, nullptr
		, 0
		, D3D11_SDK_VERSION
		, nullptr
		, &FeatureLevel
		, nullptr)))
		ctx.adapter_info.feature_level = version_t((FeatureLevel>>12) & 0xff, (FeatureLevel>>8) & 0xf);

	return ctx.adapter_info;
}

void enumerate(enumerate_fn enumerator, void* user)
{
	ref<IDXGIFactory> Factory;
	oV(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&Factory));

	int adapter_index = 0;
	ref<IDXGIAdapter> adapter;
	while (DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters(adapter_index, &adapter))
	{
		info_t adapter_info = get_info(adapter_index, adapter);

		if (!enumerator(adapter_info, user))
			return;

		adapter_index++;
		adapter = nullptr;
	}
}

version_t minimum_version(const vendor& v)
{
	switch (v)
	{
		case vendor::nvidia: return version_t(oNVVER_MAJOR, oNVVER_MINOR);
		case vendor::amd: return version_t(oAMDVER_MAJOR, oAMDVER_MINOR);
		default: break;
	}
	return version_t();
}

info_t find(const int2& virtual_desktop_position, const version_t& min_version, bool exact_version)
{
	ref<IDXGIFactory> Factory;
	oV(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&Factory));

	const bool LookForOutput = all(virtual_desktop_position != int2(oDEFAULT, oDEFAULT));

	int adapter_index = 0;
	ref<IDXGIAdapter> adapter;
	while (DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters(adapter_index, &adapter))
	{
		adapter::info_t adapter_info = get_info(adapter_index, adapter);
		version_t required_version = min_version;
		if (required_version == version_t())
			required_version = minimum_version(adapter_info.vendor);

		ref<IDXGIOutput> output;
		if (LookForOutput)
		{
			int o = 0;
			while (DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs(o, &output))
			{
				DXGI_OUTPUT_DESC od;
				output->GetDesc(&od);
				if (virtual_desktop_position.x >= od.DesktopCoordinates.left 
					&& virtual_desktop_position.x <= od.DesktopCoordinates.right 
					&& virtual_desktop_position.y >= od.DesktopCoordinates.top 
					&& virtual_desktop_position.y <= od.DesktopCoordinates.bottom)
				{
					sstring StrAdd, StrReq;
					if (exact_version)
						oCheck(adapter_info.version == required_version, std::errc::no_such_device, "Exact video driver version %s required, but current driver is %s", to_string(StrReq, required_version), to_string(StrAdd, adapter_info.version));
					else 
						oCheck(adapter_info.version >= required_version, std::errc::no_such_device, "Video driver version %s or newer required, but current driver is %s", to_string(StrReq, required_version), to_string(StrAdd, adapter_info.version));

					return adapter_info;
				}
				o++;
				output = nullptr;
			}
		}

		else if ((exact_version && adapter_info.version == required_version) || (!exact_version && adapter_info.version >= required_version))
			return adapter_info;

		adapter_index++;
		adapter = nullptr;
	}

		sstring StrReq;
	if (LookForOutput)
		oThrow(std::errc::no_such_device, "no adapter found for the specified virtual desktop coordinates that also matches the %s driver version %s", exact_version ? "exact" : "minimum", to_string(StrReq, min_version));
	else
		oThrow(std::errc::no_such_device, "no adapter found matching the %s driver version %s", exact_version ? "exact" : "minimum", to_string(StrReq, min_version));
}

info_t find(const display::id& display_id)
{
	display::info di = display::get_info(display_id);

	ref<IDXGIFactory> Factory;
	oV(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&Factory));

	int adapter_index = 0;
	ref<IDXGIAdapter> adapter;
	while (DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters(adapter_index, &adapter))
	{
		ref<IDXGIOutput> output;
		int o = 0;
		while (DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs(o, &output))
		{
			DXGI_OUTPUT_DESC od;
			output->GetDesc(&od);
			if (od.Monitor == (HMONITOR)di.native_handle)
				return get_info(adapter_index, adapter);
			o++;
			output = nullptr;
		}

		adapter_index++;
		adapter = nullptr;
	}

	oThrow(std::errc::no_such_device, "no adapter matches the specified display id");
}

}}
