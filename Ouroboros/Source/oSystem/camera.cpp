// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oCore/countof.h>
#include <oCore/ref.h>
#include <oSystem/camera.h>
#include <oSystem/module.h>
#include <oConcurrency/mutex.h>
#include <oSystem/windows/win_com.h>
#include <oSystem/windows/win_util.h>
#include <oMemory/memory.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DShow.h>
#include "winqedit.h"

using namespace std;

namespace ouro {

template<> const char* as_string(const camera::format& _Format)
{
	switch (_Format)
	{
		case camera::rgb565: return "rgb565";
		case camera::rgb24:  return "rgb24";
		case camera::argb32: return "argb32";
		case camera::rgb32:  return "rgb32";
		default: break;
	}
	return "?";
}

// 73646976-0000-0010-8000-00AA00389B71
static const GUID GUID_MEDIATYPE_Video = { 0x73646976, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
// e436eb7b-524f-11ce-9f53-0020af0ba770
static const GUID GUID_MEDIASUBTYPE_RGB565 = { 0xe436eb7b, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// e436eb7d-524f-11ce-9f53-0020af0ba770
static const GUID GUID_MEDIASUBTYPE_RGB24 = { 0xe436eb7d, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// 773c9ac0-3274-11d0-B724-00aa006c1A01
static const GUID GUID_MEDIASUBTYPE_ARGB32 = { 0x773c9ac0, 0x3274, 0x11d0, { 0xb7, 0x24, 0x0, 0xaa, 0x0, 0x6c, 0x1a, 0x1 } };
// e436eb7e-524f-11ce-9f53-0020af0ba770
static const GUID GUID_MEDIASUBTYPE_RGB32 = { 0xe436eb7e, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// e436ebb3-524f-11ce-9f53-0020af0ba770
static const GUID GUID_CLSID_FilterGraph = { 0xe436ebb3, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// BF87B6E1-8C27-11d0-B3F0-00AA003761C5
static const GUID GUID_CLSID_CaptureGraphBuilder2 = { 0xBF87B6E1, 0x8C27, 0x11d0, { 0xB3, 0xF0, 0x0, 0xAA, 0x00, 0x37, 0x61, 0xC5 } };
// 860BB310-5D01-11d0-BD3B-00A0C911CE86
static const GUID GUID_CLSID_VideoInputDeviceCategory = { 0x860BB310, 0x5D01, 0x11d0, { 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86 } };
// 62BE5D10-60EB-11d0-BD3B-00A0C911CE86
static const GUID GUID_CLSID_SystemDeviceEnum = { 0x62BE5D10, 0x60EB, 0x11d0, { 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86 } };
// C1F400A0-3F08-11d3-9F0B-006008039E37
static const GUID GUID_CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };
// 29840822-5B84-11D0-BD3B-00A0C911CE86
static const GUID GUID_IID_ICreateDevEnum = { 0x29840822, 0x5B84, 0x11D0, { 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86 } };
// 56a86895-0ad4-11ce-b03a-0020af0ba770
static const GUID GUID_IID_IBaseFilter = { 0x56a86895, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
// C6E13340-30AC-11d0-A18C-00A0C9118956
static const GUID GUID_IID_IAMStreamConfig = { 0xC6E13340, 0x30AC, 0x11d0, { 0xA1, 0x8C, 0x00, 0xA0, 0xC9, 0x11, 0x89, 0x56 } };

// The following code is from http://msdn.microsoft.com/en-us/library/dd407288(v=VS.85).aspx
namespace SampleCode {

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = nullptr;
	}
}

// Query whether a pin has a specified direction (input / output)
HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
	PIN_DIRECTION pinDir;
	HRESULT hr = pPin->QueryDirection(&pinDir);
	if (SUCCEEDED(hr))
	{
		*pResult = (pinDir == dir);
	}
	return hr;
}

// Query whether a pin is connected to another pin.
//
// Note: This function does not return a pointer to the connected pin.
static HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
{
	ref<IPin> TmpPin;
	HRESULT hr = pPin->ConnectedTo(&TmpPin);
	if (SUCCEEDED(hr))
	{
		*pResult = TRUE;
	}
	else if (hr == VFW_E_NOT_CONNECTED)
	{
		// The pin is not connected. This is not an error for our purposes.
		*pResult = FALSE;
		hr = S_OK;
	}

	return hr;
}

// Match a pin by pin direction and connection state.
static HRESULT MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult)
{
	assert(pResult != nullptr);

	BOOL bMatch = FALSE;
	BOOL bIsConnected = FALSE;

	HRESULT hr = IsPinConnected(pPin, &bIsConnected);
	if (SUCCEEDED(hr))
	{
		if (bIsConnected == bShouldBeConnected)
		{
			hr = IsPinDirection(pPin, direction, &bMatch);
		}
	}

	if (SUCCEEDED(hr))
	{
		*pResult = bMatch;
	}
	return hr;
}

// Return the first unconnected input pin or output pin.
static HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
	IEnumPins *pEnum = nullptr;
	IPin *pPin = nullptr;
	BOOL bFound = FALSE;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		goto done;
	}

	while (S_OK == pEnum->Next(1, &pPin, nullptr))
	{
		hr = MatchPin(pPin, PinDir, FALSE, &bFound);
		if (FAILED(hr))
		{
			goto done;
		}
		if (bFound)
		{
			*ppPin = pPin;
			(*ppPin)->AddRef();
			break;
		}
		SafeRelease(&pPin);
	}

	if (!bFound)
	{
		hr = VFW_E_NOT_FOUND;
	}

done:
	SafeRelease(&pPin);
	SafeRelease(&pEnum);
	return hr;
}

// Connect output pin to filter.
static HRESULT connect_filters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest)
{
	IPin *pIn = nullptr;

	// Find an input pin on the downstream filter.
	HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	if (SUCCEEDED(hr))
	{
		// Try to connect them.
		hr = pGraph->Connect(pOut, pIn);
		pIn->Release();
	}
	return hr;
}

// http://msdn.microsoft.com/en-us/library/dd375432(v=vs.85).aspx
// Release the format block for a media type.
void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = nullptr;
	}
	if (mt.pUnk != nullptr)
	{
		// pUnk should not be used.
		mt.pUnk->Release();
		mt.pUnk = nullptr;
	}
}

// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != nullptr)
	{
		_FreeMediaType(*pmt); 
		CoTaskMemFree(pmt);
	}
}

} // namespace SampleCode

static GUID media_subtype(camera::format _Format)
{
	switch (_Format)
	{
		case camera::rgb565: return (const GUID&)GUID_MEDIASUBTYPE_RGB565;
		case camera::rgb24: return (const GUID&)GUID_MEDIASUBTYPE_RGB24;
		case camera::argb32: return (const GUID&)GUID_MEDIASUBTYPE_ARGB32;
		case camera::rgb32: return (const GUID&)GUID_MEDIASUBTYPE_RGB32;
		default: break;
	}
	return GUID_NULL;
}

struct DSSampleGrabberCB : ISampleGrabberCB
{
	// Bridge object to avoid multiple inheritance. Basically this is going to 
	// delegate all functionality back to the specified directshow_camera.
	DSSampleGrabberCB(class directshow_camera* _pDSCamera) : pDSCamera(_pDSCamera) {}
	ULONG STDMETHODCALLTYPE AddRef(void) override { return 1; }
	ULONG STDMETHODCALLTYPE Release(void) override { return 1; }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) override { return E_NOINTERFACE; }
	HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample* pSample) override { return E_FAIL; }
	HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) override;
	class directshow_camera* pDSCamera;
};

class directshow_camera : public camera
{
public:
	directshow_camera(IGraphBuilder* _pGraphBuilder
		, IAMStreamConfig* _pStreamConfig
		, IBaseFilter* _pBaseFilter
		, const char* _Name
		, unsigned int _ID);

	~directshow_camera();

	const char* name() const override;
	unsigned int id() const override;
	float fps() const override;
	mode get_mode() const override;
	void set_mode(const mode& _Mode) override;
	bool capturing() const override;
	void capturing(bool _Capturing) override;

	// Return true from _Enumerator to continue, or false to abort enumeration.
	void enumerate_modes(const std::function<bool(const mode& _Mode)>& _Enumerator) const override;

	// returns mode() if none found
	mode find_closest_matching(const mode& _Mode) const override;

	bool map_const(const_mapped* _pMapped) const override;
	void unmap_const() const override;

protected:
	friend struct DSSampleGrabberCB;

	bool BufferCB(double _SampleTime, void* _pBuffer, size_t _SizeofBuffer);
	unsigned int CalculateSourceRowPitch() const;
	void destroy_output();
	void resize_target(const mode& _Mode);
	void recreate_output(const mode& _Mode);
	void get_set_mode_list(unsigned int* _pNumModes, mode* _pModes, const mode* _pNewMode);
	void mode_list(unsigned int* _pNumModes, mode* _pModes) const { const_cast<directshow_camera*>(this)->get_set_mode_list(_pNumModes, _pModes, nullptr); }

	DSSampleGrabberCB SampleGrabberCB;
	ref<ISampleGrabber> SampleGrabber;
	ref<IBaseFilter> VideoInput;
	ref<IBaseFilter> VideoOutput;
	ref<IGraphBuilder> GraphBuilder;
	ref<IAMStreamConfig> StreamConfig;

	mode Mode;
	typedef shared_mutex mutex_t;
	typedef lock_guard<mutex_t> lock_t;
	typedef shared_lock<mutex_t> lock_shared_t;
	mutable mutex_t Mutex;
	std::atomic<uint> RingBufferReadIndex;
	unsigned int MonotonicCounter;
	unsigned int ID;
	uri_string Name;
	volatile bool Running;

	struct frame
	{
		frame()
			: SampleTime(0.0)
			, Frame(0)
		{}

		double SampleTime;
		unsigned int Frame;
		std::vector<unsigned char> Data;
	};

	frame RingBuffer[8];
};

directshow_camera::directshow_camera(IGraphBuilder* _pGraphBuilder, IAMStreamConfig* _pStreamConfig, IBaseFilter* _pBaseFilter, const char* _Name, unsigned int _ID)
	: GraphBuilder(_pGraphBuilder)
	, VideoInput(_pBaseFilter)
	, StreamConfig(_pStreamConfig)
	#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
		, SampleGrabberCB(this)
	#pragma warning(default:4355)
	, RingBufferReadIndex(0)
	, MonotonicCounter(0)
	, Name(_Name)
	, Running(true)
	, ID(_ID)
{
	// Find and set the first mode in the list of supported modes to initialize
	// the ring buffer and put the graph in a RUN state.
	unsigned int nModes = 0;
	directshow_camera::mode_list(&nModes, nullptr);
	mode* modes = (mode*)alloca(nModes * sizeof(mode));
	directshow_camera::mode_list(&nModes, modes);
	directshow_camera::set_mode(modes[nModes-1]); // start in the highest-fidelity mode
}

directshow_camera::~directshow_camera()
{
	ref<IMediaControl> MediaControl;
	oV_NOTHROW(GraphBuilder->QueryInterface(IID_PPV_ARGS(&MediaControl)));
	MediaControl->Stop();
}

void directshow_camera::resize_target(const mode& _Mode)
{
	MonotonicCounter = 0;
	RingBufferReadIndex.exchange(0);
	size_t frameSize = surface::mip_size(surface::format(_Mode.format), _Mode.dimensions);
	for (auto& r : RingBuffer)
	{
		r.SampleTime = 0.0;
		r.Data.resize(frameSize);
	}
}

void directshow_camera::destroy_output()
{
	SampleGrabber = nullptr;
	if (VideoOutput)
	{
		oV(GraphBuilder->RemoveFilter(VideoOutput));
		VideoOutput = nullptr;
	}
}

void directshow_camera::recreate_output(const mode& _Mode)
{
	if (GUID_NULL == media_subtype(_Mode.format))
		throw std::invalid_argument(std::string("Unsupported format ") + as_string(_Mode.format));

	// Create an output node and attach it to the input
	CoCreateInstance((const GUID&)GUID_CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&VideoOutput));
	oV(GraphBuilder->AddFilter(VideoOutput, L"Sample Grabber"));
	ref<IEnumPins> EnumPins;
	if (FAILED(VideoInput->EnumPins(&EnumPins)))
		throw windows::error();

	bool connectedFilters = false;
	ref<IPin> Pin;
	while (S_OK == EnumPins->Next(1, &Pin, nullptr))
	{
		if (SUCCEEDED(SampleCode::connect_filters(GraphBuilder, Pin, VideoOutput)))
		{
			connectedFilters = true;
			break;
		}
	}
	oCheck(connectedFilters, std::errc::function_not_supported, "Could not connect VideoOutput filter");

	// Now get it as a SampleGrabber and set up its parameters
	oV(VideoOutput->QueryInterface(IID_PPV_ARGS(&SampleGrabber)));
	oV(SampleGrabber->SetCallback(&SampleGrabberCB, 1));

	AM_MEDIA_TYPE outputType;
	memset(&outputType, 0, sizeof(outputType));
	outputType.majortype = (const GUID&)GUID_MEDIATYPE_Video;
	outputType.subtype = media_subtype(_Mode.format);
	oCheck(outputType.subtype != GUID_NULL, std::errc::not_supported, "Unsupported format: %s", as_string(_Mode.format));

	oV(SampleGrabber->SetMediaType(&outputType));
}

const char* directshow_camera::name() const
{
	return Name;
}

unsigned int directshow_camera::id() const
{
	return ID;
}

float directshow_camera::fps() const
{
	const frame& f = RingBuffer[RingBufferReadIndex]; // const read and RingBufferReadIndex is updated atomically
	return static_cast<float>(f.Frame / __max(f.SampleTime, 0.00001f));
}

camera::mode directshow_camera::get_mode() const
{
	lock_shared_t lock(Mutex);
	return Mode;
}

void directshow_camera::set_mode(const mode& _Mode)
{
	if (GUID_NULL == media_subtype(_Mode.format))
		oThrow(std::errc::not_supported, "Unsupported format: %s", as_string(_Mode.format));

	mode closest = find_closest_matching(_Mode);

	if (memcmp(&closest, &_Mode, sizeof(mode)))
		oThrow(std::errc::not_supported, "Unsupported mode specified: %s %dx%d", as_string(_Mode.format), _Mode.dimensions.x, _Mode.dimensions.y);

	lock_t lock(Mutex);

	ref<IMediaControl> MediaControl;
	oV(GraphBuilder->QueryInterface(IID_PPV_ARGS(&MediaControl)));

	// Prevent deadlock in BufferCB, we've got an exclusive above
	bool PriorRunning = Running;
	Running = false;
	if (PriorRunning)
	{
		MediaControl->StopWhenReady();
		this_thread::sleep_for(chrono::milliseconds(500));
	}

	destroy_output();
	unsigned int nModes = 0;
	get_set_mode_list(&nModes, nullptr, &_Mode);

	resize_target(_Mode);
	recreate_output(_Mode);

	Mode = _Mode;

	Running = PriorRunning;
	if (Running)
		MediaControl->Run();
}

bool directshow_camera::capturing() const
{
	return Running;
}

void directshow_camera::capturing(bool _Capturing)
{
	lock_t lock(Mutex);

	ref<IMediaControl> MediaControl;
	oV(GraphBuilder->QueryInterface(IID_PPV_ARGS(&MediaControl)));

	Running = _Capturing;

	if (_Capturing)
		MediaControl->Run();
	else
		MediaControl->Stop();
}

static bool operator<(const camera::mode& _This, const camera::mode& _That)
{
	auto b1 = surface::channel_bits(surface::format(_This.format));
	auto b2 = surface::channel_bits(surface::format(_That.format));
	if (b1.r >= b2.r) return false;
	if (b1.g >= b2.g) return false;
	if (b1.b >= b2.b) return false;
	if (b1.a >= b2.a) return false;
	if (!surface::is_block_compressed(surface::format(_This.format)) && surface::is_block_compressed(surface::format(_That.format))) return false;

	size_t thisSize = surface::element_size(surface::format(_This.format));
	size_t thatSize = surface::element_size(surface::format(_That.format));
	if (thisSize < thatSize)
		return true;

	if (thisSize == thatSize)
	{
		if (_This.dimensions.x < _That.dimensions.x)
			return true;

		if (_This.dimensions.x == _This.dimensions.x)
		{
			if (_This.dimensions.y < _That.dimensions.y)
				return true;

			if (_This.dimensions.y == _That.dimensions.y)
			{
				if (_This.bit_rate < _That.bit_rate)
					return true;
			}
		}
	}

	return false;
}

template<typename T> inline T sqDiff(const T& _A, const T& _B)
{
	T t = _A - _B;
	return t * t;
}

static float distance(const camera::mode& _ModeToMatch, const camera::mode _TestMode)
{
	static const float kFormatWeight = 0.33f;
	static const float kWidthWeight = 0.33f;
	static const float kHeightWeight = 0.33f;
	static const float kBitRateWeight = 0.0f;

	auto b1 = surface::channel_bits(surface::format(_ModeToMatch.format));
	auto b2 = surface::channel_bits(surface::format(_TestMode.format));

	int FormatDiff = sqDiff(b1.r, b2.r) + sqDiff(b1.g, b2.g) + sqDiff(b2.b, b2.b) + sqDiff(b1.a, b2.a);
	int2 SizeDiff = sqDiff(_ModeToMatch.dimensions, _TestMode.dimensions);
	int BitRateDiff = sqDiff(_ModeToMatch.bit_rate, _TestMode.bit_rate);

	return kFormatWeight*FormatDiff + kWidthWeight*SizeDiff.x + kHeightWeight*SizeDiff.y + kBitRateWeight*BitRateDiff;
}

void directshow_camera::enumerate_modes(const std::function<bool(const mode& _Mode)>& _Enumerator) const
{
	unsigned int nModes = 0;
	mode_list(&nModes, nullptr);

	mode* modes = (mode*)alloca(nModes * sizeof(mode));
	mode_list(&nModes, modes);

	for (unsigned int i = 0; i < nModes; i++)
		if (!_Enumerator(modes[i]))
			break;
}

camera::mode directshow_camera::find_closest_matching(const mode& _ModeToMatch) const
{
	unsigned int nModes = 0;
	mode_list(&nModes, nullptr);

	mode* modes = (mode*)alloca(nModes * sizeof(mode));
	mode_list(&nModes, modes);

	float minDistance = std::numeric_limits<float>::max();
	unsigned int minIndex = ~0u;
	for (unsigned int i = 0; i < nModes; i++)
	{
		float d = distance(_ModeToMatch, modes[i]);
		if (d <= minDistance) // <= ensures we use the farthest into the list i.e. the highest quality
		{
			minDistance = d;
			minIndex = i;
		}
	}

	return modes[minIndex];
}

surface::format get_format(const BITMAPINFOHEADER& _BitmapInfoHeader)
{
	switch (_BitmapInfoHeader.biBitCount)
	{
		case 1: return surface::format::r1_unorm;
		case 16: return surface::format::b5g5r5a1_unorm; // not sure if alpha is respected/but there is no B5G5R5X1_UNORM currently
		case 0:
		case 24: return surface::format::b8g8r8_unorm;
		case 32: return surface::format::b8g8r8x8_unorm;
		default: break;
	}
	return surface::format::unknown; // no format for paletted types currently
}

void directshow_camera::get_set_mode_list(unsigned int* _pNumModes, mode* _pModes, const mode* _pNewMode)
{
	if (!_pNumModes && (!_pNewMode || (_pNewMode && !_pModes)))
		throw std::invalid_argument("invalid argument");

	*_pNumModes = 0;
	int nCapabilities = 0;
	int sizeofConfigStruct = 0;
	oV(StreamConfig->GetNumberOfCapabilities(&nCapabilities, &sizeofConfigStruct));
	oAssert(sizeofConfigStruct == sizeof(VIDEO_STREAM_CONFIG_CAPS), "");
	oCheck(nCapabilities, std::errc::not_supported, "");

	mode testMode;
	VIDEO_STREAM_CONFIG_CAPS scc;
	for (int i = 0; i < nCapabilities; i++)
	{
		AM_MEDIA_TYPE* type = nullptr;
		oV(StreamConfig->GetStreamCaps(i, &type, (BYTE*)&scc));
		VIDEOINFOHEADER* pVideoHeader = (VIDEOINFOHEADER*)type->pbFormat;
		if (type->majortype == (const GUID&)GUID_MEDIATYPE_Video && pVideoHeader->bmiHeader.biWidth > 0 && pVideoHeader->bmiHeader.biHeight > 0)
		{
			surface::format Format = get_format(pVideoHeader->bmiHeader);

			if (Format != surface::format::unknown)
			{
				mode& m = _pModes ? _pModes[(*_pNumModes)] : testMode;
				m.format = camera::format(Format);
				m.dimensions = int2(pVideoHeader->bmiHeader.biWidth, pVideoHeader->bmiHeader.biHeight);
				m.bit_rate = static_cast<int>(pVideoHeader->dwBitRate);
				if (_pNewMode && !memcmp(_pNewMode, &m, sizeof(mode)))
				{
					oV(StreamConfig->SetFormat(type));
					SampleCode::_DeleteMediaType(type);
					return;
				}

				(*_pNumModes)++;
			}
			
			SampleCode::_DeleteMediaType(type);
		}
	}

	if (_pModes && *_pNumModes)
		std::sort(_pModes, _pModes + *_pNumModes);

	if (_pNewMode)
		oThrow(std::errc::not_supported, "Mode not valid %s %dx%d", as_string(_pNewMode->format), _pNewMode->dimensions.x, _pNewMode->dimensions.y);
}

bool directshow_camera::map_const(const_mapped* _pMapped) const
{
	if (Running)
	{
		Mutex.lock_shared();
		unsigned int index = RingBufferReadIndex;
		
		surface::info_t si;
		si.dimensions = int3(Mode.dimensions, 1);
		si.format = surface::format(Mode.format);
		_pMapped->data = RingBuffer[index].Data.data();
		_pMapped->row_pitch = surface::row_pitch(si);
		_pMapped->frame = RingBuffer[index].Frame;
	}

	return Running;
}

void directshow_camera::unmap_const() const
{
	Mutex.unlock_shared();
}

HRESULT DSSampleGrabberCB::BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen)
{
	return pDSCamera->BufferCB(SampleTime, pBuffer, BufferLen) ? S_OK : E_FAIL;
}

unsigned int directshow_camera::CalculateSourceRowPitch() const
{
	// BITMAPINFOHEADER rules, BITMAPs are aligned to 4 pixels per row when 
	// allocating the image size.
	return align(Mode.dimensions.x, 4) * surface::element_size(surface::format(Mode.format));
}

bool directshow_camera::BufferCB(double _SampleTime, void* _pBuffer, size_t _SizeofBuffer)
{
	if (Running && _SizeofBuffer > 0)
	{
		lock_shared_t lock(Mutex);

		int WriteIndex = (RingBufferReadIndex + 1) % countof(RingBuffer);
	
		frame& f = RingBuffer[WriteIndex];
		if (_SizeofBuffer != f.Data.size())
			oThrow(std::errc::no_buffer_space, "Mismatched buffer size. Got %u bytes, expected %u bytes", _SizeofBuffer, f.Data.size());
		
		if (_SizeofBuffer != f.Data.size())
			return false;
		
		f.Frame = MonotonicCounter++;
		f.SampleTime = _SampleTime;
		
		surface::info_t si;
		si.dimensions = int3(Mode.dimensions, 1); 
		si.format = surface::format(Mode.format);

		int DestRowPitch = surface::row_pitch(si);
		int DestRowSize = surface::row_size(si.format, si.dimensions);
		int NumRows = surface::num_rows(si.format, si.dimensions);
		memcpy2dvflip(f.Data.data(), DestRowPitch, _pBuffer, CalculateSourceRowPitch(), DestRowSize, NumRows);
		RingBufferReadIndex.exchange(WriteIndex);
	}

	return true;
}

namespace directshow {

static void create_capture_graph_builder(ICaptureGraphBuilder2** _ppCaptureGraphBuilder, IGraphBuilder** _ppGraphBuilder)
{
	CoCreateInstance((const GUID&)GUID_CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(_ppCaptureGraphBuilder));
	CoCreateInstance((const GUID&)GUID_CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,IID_PPV_ARGS(_ppGraphBuilder));
	(*_ppCaptureGraphBuilder)->SetFiltergraph(*_ppGraphBuilder);
}

static void moniker_enumerate(unsigned int _Index, IMoniker** _ppMoniker)
{
	ref<ICreateDevEnum> DevEnum;
	oV(CoCreateInstance((const GUID&)GUID_CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, (const GUID&)GUID_IID_ICreateDevEnum, (void**)&DevEnum));

	ref<IEnumMoniker> EnumMoniker;
	HRESULT hr = DevEnum->CreateClassEnumerator((const GUID&)GUID_CLSID_VideoInputDeviceCategory, &EnumMoniker, 0);
	if (S_OK != hr)
	{
		module::info mi = this_module::get_info();
		oThrow(std::errc::no_such_device, "No video input devices found%s", mi.is_64bit_binary ? " (might be because of a 32-bit driver on 64-bit Windows)" : "");
	}

	std::vector<ref<IMoniker>> Monikers;
	Monikers.resize(_Index + 1);

	ULONG nFetched = 0;
	if (FAILED(EnumMoniker->Next(static_cast<unsigned int>(Monikers.size()), &Monikers[0], &nFetched)) || (nFetched <= _Index))
		oThrow(std::errc::no_such_device, "");

	*_ppMoniker = Monikers[_Index];
	(*_ppMoniker)->AddRef();
}

void get_string_property(char* _StrDestination, size_t _SizeofStrDestination, IMoniker* _pMoniker, const char* _Property)
{
	ref<IPropertyBag> PropertyBag;
	oV(_pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&PropertyBag));
	VARIANT varName;
	VariantInit(&varName);
	lwstring PropertyName = _Property;
	oV(PropertyBag->Read(PropertyName.c_str(), &varName, 0));
	if (wcsltombs(_StrDestination, varName.bstrVal, _SizeofStrDestination) >= _SizeofStrDestination)
		oThrow(std::errc::no_buffer_space, "");
	VariantClear(&varName);
}

void get_display_name(char* _StrDestination, size_t _SizeofStrDestination, IMoniker* _pMoniker)
{
	LPOLESTR lpDisplayName = nullptr;
	oV(_pMoniker->GetDisplayName(nullptr, nullptr, &lpDisplayName));
	if (wcsltombs(_StrDestination, lpDisplayName, _SizeofStrDestination) >= _SizeofStrDestination)
		oThrow(std::errc::no_buffer_space, "");
	ref<IMalloc> Malloc;
	oV(CoGetMalloc(1, &Malloc));
	Malloc->Free(lpDisplayName);
}

} // namespace directshow

static bool make(unsigned int _Index, std::shared_ptr<camera>& _Camera)
{
	_Camera = nullptr;

	windows::com::ensure_initialized();

	ref<IMoniker> Moniker;
	
	try { directshow::moniker_enumerate(_Index, &Moniker); }
	catch (std::system_error& e) { if (e.code() == std::errc::no_such_device) return false; else throw e; }

	uri_string Name;
	directshow::get_string_property(Name.c_str(), Name.capacity(), Moniker, "FriendlyName");
	//mstring Description;
	//directshow_get_string_property(Description.c_str(), Description.capacity(), Moniker, "Description");
	//mstring DevicePath;
	//directshow_get_string_property(DevicePath.c_str(), DevicePath.capacity(), Moniker, "DevicePath");
	// This comes out more consistently than DevicePath, but we use ID below
	//mstring DisplayName;
	//oVERIFY(directshow_get_display_name(DisplayName.c_str(), DisplayName.capacity(), Moniker));
	unsigned int MonikerID = 0;
	oV(Moniker->Hash((DWORD*)&MonikerID));

	ref<IBaseFilter> BaseFilter;
	if (FAILED(Moniker->BindToObject(nullptr, nullptr, (const GUID&)GUID_IID_IBaseFilter, (void**)&BaseFilter)))
		oThrow(std::errc::function_not_supported, "Found device \"%s\" (ID=0x%x), but could not bind an interface for it", Name.c_str(), MonikerID);

	//bool running = Moniker->IsRunning(nullptr, nullptr, nullptr) == S_OK;

	oTrace("Found device \"%s\" (ID=0x%x), continuing...", Name.c_str(), MonikerID);

	ref<ICaptureGraphBuilder2> CaptureGraphBuilder;
	ref<IGraphBuilder> GraphBuilder;
	directshow::create_capture_graph_builder(&CaptureGraphBuilder, &GraphBuilder);

	oV(GraphBuilder->AddFilter(BaseFilter, L"Video Input"));
	ref<IAMStreamConfig> AMStreamConfig;
	if (SUCCEEDED(CaptureGraphBuilder->FindInterface(nullptr, (const GUID*)&GUID_MEDIATYPE_Video, BaseFilter, (const GUID&)GUID_IID_IAMStreamConfig, (void**)&AMStreamConfig)))
	{
		_Camera = std::make_shared<directshow_camera>(GraphBuilder, AMStreamConfig, BaseFilter, Name, MonikerID);
		return true;
	}
	return false;
}

void camera::enumerate(const std::function<bool(std::shared_ptr<camera> _Camera)>& _Enumerator)
{
#if 0
	module::info mi = this_module::get_info();
	if (mi.is_64bit_binary)
		oThrow(std::errc::not_supported, 
			"camera is not supported in 64-bit builds because of the lack of valid "
			"64-bit camera drivers or a way of reliably determining if a camera returned "
			"uses 32- or 64-bit drivers.");
#endif

	unsigned int index = ~0u;
	std::shared_ptr<camera> _Camera;
	while (make(++index, _Camera))
		_Enumerator(_Camera);
}

}
