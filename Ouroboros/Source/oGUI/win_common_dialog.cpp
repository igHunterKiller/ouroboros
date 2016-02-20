// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/Windows/win_common_dialog.h>
#include <oGUI/Windows/win_gdi.h>
#include <Commdlg.h>
#include <CdErr.h>

namespace ouro {
	namespace windows {
		namespace common_dialog {

static const char* as_string_CD_err(int _CDERRCode)
{
	switch (_CDERRCode)
	{
		case CDERR_DIALOGFAILURE: return "CDERR_DIALOGFAILURE";
		case CDERR_GENERALCODES: return "CDERR_GENERALCODES";
		case CDERR_STRUCTSIZE: return "CDERR_STRUCTSIZE";
		case CDERR_INITIALIZATION: return "CDERR_INITIALIZATION";
		case CDERR_NOTEMPLATE: return "CDERR_NOTEMPLATE";
		case CDERR_NOHINSTANCE: return "CDERR_NOHINSTANCE";
		case CDERR_LOADSTRFAILURE: return "CDERR_LOADSTRFAILURE";
		case CDERR_FINDRESFAILURE: return "CDERR_FINDRESFAILURE";
		case CDERR_LOADRESFAILURE: return "CDERR_LOADRESFAILURE";
		case CDERR_LOCKRESFAILURE: return "CDERR_LOCKRESFAILURE";
		case CDERR_MEMALLOCFAILURE: return "CDERR_MEMALLOCFAILURE";
		case CDERR_MEMLOCKFAILURE: return "CDERR_MEMLOCKFAILURE";
		case CDERR_NOHOOK: return "CDERR_NOHOOK";
		case CDERR_REGISTERMSGFAIL: return "CDERR_REGISTERMSGFAIL";
		case PDERR_PRINTERCODES: return "PDERR_PRINTERCODES";
		case PDERR_SETUPFAILURE: return "PDERR_SETUPFAILURE";
		case PDERR_PARSEFAILURE: return "PDERR_PARSEFAILURE";
		case PDERR_RETDEFFAILURE: return "PDERR_RETDEFFAILURE";
		case PDERR_LOADDRVFAILURE: return "PDERR_LOADDRVFAILURE";
		case PDERR_GETDEVMODEFAIL: return "PDERR_GETDEVMODEFAIL";
		case PDERR_INITFAILURE: return "PDERR_INITFAILURE";
		case PDERR_NODEVICES: return "PDERR_NODEVICES";
		case PDERR_NODEFAULTPRN: return "PDERR_NODEFAULTPRN";
		case PDERR_DNDMMISMATCH: return "PDERR_DNDMMISMATCH";
		case PDERR_CREATEICFAILURE: return "PDERR_CREATEICFAILURE";
		case PDERR_PRINTERNOTFOUND: return "PDERR_PRINTERNOTFOUND";
		case PDERR_DEFAULTDIFFERENT: return "PDERR_DEFAULTDIFFERENT";
		case CFERR_CHOOSEFONTCODES: return "CFERR_CHOOSEFONTCODES";
		case CFERR_NOFONTS: return "CFERR_NOFONTS";
		case CFERR_MAXLESSTHANMIN: return "CFERR_MAXLESSTHANMIN";
		case FNERR_FILENAMECODES: return "FNERR_FILENAMECODES";
		case FNERR_SUBCLASSFAILURE: return "FNERR_SUBCLASSFAILURE";
		case FNERR_INVALIDFILENAME: return "FNERR_INVALIDFILENAME";
		case FNERR_BUFFERTOOSMALL: return "FNERR_BUFFERTOOSMALL";
		case FRERR_FINDREPLACECODES: return "FRERR_FINDREPLACECODES";
		case FRERR_BUFFERLENGTHZERO: return "FRERR_BUFFERLENGTHZERO";
		case CCERR_CHOOSECOLORCODES: return "CCERR_CHOOSECOLORCODES";
		default: break;
	}

	return "unrecognized CDERR";
}

static bool open_or_save_path(path_t& _Path, const char* _DialogTitle, const char* _FilterPairs, HWND _hParent, bool _IsOpenNotSave)
{
	path_string StrPath;
	path_string InitDir;

	const bool kHasFilename = _Path.has_filename();

	// removes trailing separator which confuses win GetOpenFileName
	if (kHasFilename)
		oCheck(_Path.empty() || clean_path(StrPath, _Path, '\\'), std::errc::invalid_argument, "bad path: %s", _Path.c_str());

	else
	{
		path_t copy(_Path);
		copy.remove_filename(); // removes trailing separators
		oCheck(copy.empty() || clean_path(InitDir, copy, '\\'), std::errc::invalid_argument, "bad path: %s", _Path.c_str());
	}

	std::string filters;
	char defext[4] = {0};
	if (_FilterPairs)
	{
		filters.assign(_FilterPairs, _FilterPairs+strlen(_FilterPairs)+1);
		filters.append("x"); // set up double nul ptr
		filters[filters.size()-1] = 0;
		for (size_t i = 0; i < filters.size(); i++)
			if (filters[i] == '|')
				filters[i] = '\0';

		// use first ext as default ext...
		const char* first = filters.c_str() + strlen(filters.c_str())+1;
		first = strchr(first, '.');
		if (first)
		{
			defext[0] = *(first+1);
			defext[1] = *(first+2);
			defext[2] = *(first+3);
		}
	}

	OPENFILENAMEA o = {0};
	o.lStructSize = sizeof(OPENFILENAMEA);
	o.hwndOwner = _hParent;
	o.hInstance = nullptr;
	o.lpstrFilter = _FilterPairs ? filters.c_str() : nullptr;
	o.lpstrCustomFilter = nullptr;
	o.nMaxCustFilter = 0;
	o.nFilterIndex = 1;
	o.lpstrFile = StrPath;
	o.nMaxFile = static_cast<DWORD>(StrPath.capacity());
	o.lpstrFileTitle = nullptr;
	o.nMaxFileTitle = 0;
	o.lpstrInitialDir = InitDir.empty() ? nullptr : InitDir;
	o.lpstrTitle = _DialogTitle;

	if (_IsOpenNotSave)
		o.Flags = OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
	else
		OFN_OVERWRITEPROMPT;
	
	o.nFileOffset = 0;
	o.nFileExtension = 0;
	o.lpstrDefExt = _FilterPairs ? defext : nullptr;
	o.lCustData = (LPARAM)nullptr;
	o.lpfnHook = nullptr;
	o.lpTemplateName = nullptr;
	o.pvReserved = nullptr;
	o.dwReserved = 0;
	o.FlagsEx = 0;

	bool success = _IsOpenNotSave ? !!GetOpenFileName(&o) : !!GetSaveFileName(&o);
	if (!success)
	{
		DWORD err = CommDlgExtendedError();
		if (err == CDERR_GENERALCODES)
			return false;
		oThrow(std::errc::protocol_error, "%s", as_string_CD_err(err));
	}

	_Path = StrPath;
	return true;
}

bool open_path(path_t& _Path, const char* _DialogTitle, const char* _FilterPairs, HWND _hParent)
{
	return open_or_save_path(_Path, _DialogTitle, _FilterPairs, _hParent, true);
}

bool save_path(path_t& _Path, const char* _DialogTitle, const char* _FilterPairs, HWND _hParent)
{
	return open_or_save_path(_Path, _DialogTitle, _FilterPairs, _hParent, false);
}

bool pick_color(uint32_t* inout_argb, HWND hparent)
{
	argb_channels ch;
	ch.argb = *inout_argb;

	std::array<COLORREF, 16> custom;
	custom.fill(RGB(255,255,255));

	CHOOSECOLOR cc = {0};
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hparent;
	cc.rgbResult = RGB(ch.r,ch.g,ch.b);
	cc.lpCustColors = custom.data();
	cc.Flags = CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT|CC_SOLIDCOLOR;

	if (!ChooseColor(&cc))
	{
		DWORD err = CommDlgExtendedError();
		if (err == CDERR_GENERALCODES)
			return false;
		oThrow(std::errc::protocol_error, "%s", as_string_CD_err(err));
	}

	ch.r = GetRValue(cc.rgbResult);
	ch.g = GetGValue(cc.rgbResult);
	ch.b = GetBValue(cc.rgbResult);

	*inout_argb = ch.argb;
	return true;
}

bool pick_font(LOGFONT* _pLogicalFont, uint32_t* inout_argb, HWND _hParent)
{
	gdi::scoped_getdc hDC(_hParent);
	CHOOSEFONT cf = {0};
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = _hParent;
	cf.hDC = hDC;
	cf.lpLogFont = _pLogicalFont;
	cf.iPointSize = 0;
	cf.Flags = CF_EFFECTS|CF_INITTOLOGFONTSTRUCT;

	argb_channels ch;
	if (inout_argb)
	{
		ch.argb = *inout_argb;
		cf.rgbColors = RGB(ch.r,ch.g,ch.b);
	}
	else
		cf.rgbColors = RGB(0,0,0);

	cf.nFontType = SCREEN_FONTTYPE;

	if (!ChooseFont(&cf))
	{
		DWORD err = CommDlgExtendedError();
		if (err == CDERR_GENERALCODES)
			return false;
		oThrow(std::errc::protocol_error, "%s", as_string_CD_err(err));
	}

	if (inout_argb)
	{
		ch.r = GetRValue(cf.rgbColors);
		ch.g = GetGValue(cf.rgbColors);
		ch.b = GetBValue(cf.rgbColors);
		*inout_argb = ch.argb;
	}

	return true;
}
		} // namespace common_dialog
	}
}