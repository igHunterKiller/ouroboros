// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/algorithm.h>
#include <oGUI/Windows/oWinControlSet.h>
#include <oSystem/windows/win_error.h>
#include <oGUI/Windows/win_gdi_draw.h>
#include <oGUI/Windows/oWinRect.h>

using namespace ouro;
using namespace ouro::windows::gdi;

void oWinControlSet::Initialize(HWND _hParent, const int2& _ParentClientSize, const xml& _XML, const IDFROMSTRING& _IDFromString)
{
	if (!_hParent)
		oThrow(std::errc::invalid_argument, "A valid HWND must be specified as the parent");

	xml::node hRoot = _XML.first_child(_XML.root(), "oUI");
	if (!hRoot)
		oThrow(std::errc::invalid_argument, "XML root node \"oUI\" not found");

	fonts_t fonts;
	CreateFontsSibling(_XML, &fonts);

	CONTROL_CONTEXT cc;
	cc.hParent = _hParent;
	cc.ParentSize = _ParentClientSize;
	cc.IDFromString = _IDFromString;

	XML_CONTEXT xc(fonts);
	xc.pXML = &_XML;
	xc.hNode = hRoot;

	CreateControlsSibling(xc, cc, &Controls);
}

void oWinControlSet::Deinitialize()
{
	for (HWND hWnd : Controls)
		if (IsWindow(hWnd))
			DestroyWindow(hWnd);

	Controls.clear();
}

HWND oWinControlSet::GetControl(int _ID) const
{
	if (_ID < 0 || _ID >= (int)Controls.size())
		return nullptr;
	return Controls[_ID];
}

void oWinControlSet::CreateFontsSibling(const xml& _XML, fonts_t* _pFonts)
{
	xml::node hRoot = _XML.first_child(_XML.root(), "oUI");
	if (!hRoot)
		oThrow(std::errc::invalid_argument, "XML root node \"oUI\" not found");

	xml::node hChild = _XML.first_child(hRoot, "Font");
	while (hChild)
	{
		const char* StrFont = _XML.find_attr_value(hChild, "Name");
		oCheck(StrFont, std::errc::invalid_argument, "Invalid or missing Name attribute in a Font node");
		oCheck(_pFonts->end() == _pFonts->find(StrFont), std::errc::invalid_argument, "Font %s already defined", StrFont);

		ouro::font_info fd;
		ParseFontDesc(_XML, hChild, &fd);
		(*_pFonts)[StrFont] = make_font(fd);
		hChild = _XML.next_sibling(hChild, "Font");
	}
}

// @tony: Should this be promoted to somewhere more generic?
#define oXML_GETVALUE(_XML, _hNode, _pDestStruct, _FieldName) _XML.find_attr_value(_hNode, #_FieldName, &_pDestStruct->_FieldName);

bool oWinControlSet::ParseFontDesc(const xml& _XML, xml::node _hNode, ouro::font_info* _pDesc)
{
	oXML_GETVALUE(_XML, _hNode, _pDesc, name);
	oXML_GETVALUE(_XML, _hNode, _pDesc, point_size);
	oXML_GETVALUE(_XML, _hNode, _pDesc, bold);
	oXML_GETVALUE(_XML, _hNode, _pDesc, italic);
	oXML_GETVALUE(_XML, _hNode, _pDesc, underline);
	oXML_GETVALUE(_XML, _hNode, _pDesc, strikeout);
	return true;
}

bool oWinControlSet::ParseControlDesc(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, const controls_t& _Controls, ouro::control_info* _pDesc)
{
	_pDesc->parent = (ouro::window_handle)_ControlContext.hParent;
	_pDesc->font = (ouro::font_handle)_ControlContext.hFont;

	const char* StrID = _XmlContext.pXML->find_attr_value(_XmlContext.hNode, "ID");
	if (!oSTRVALID(StrID))
		oThrow(std::errc::invalid_argument, "Invalid or missing ID attribute for the specified Control.");

	int ID = -1;
	if (_ControlContext.IDFromString(&ID, StrID))
		_pDesc->id = (uint16_t)ID;
	else
		oThrow(std::errc::invalid_argument, "Undeclared ID %s. All IDs must be declared as an enum in code.", StrID);

	if (ID < (int)_Controls.size() && _Controls[ID])
		oThrow(std::errc::invalid_argument, "ID %s has already been used", StrID);

	if (!_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Type", &_pDesc->type))
		oThrow(std::errc::invalid_argument, "Invalid or missing Type attribute for Control %s", StrID);

	_pDesc->text = oSAFESTR(_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Text"));
	_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "StartsNewGroup", &_pDesc->starts_new_group); // allow this to default to false

	const char* StrFont = _XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Font"); // allow this to default to the last context's font
	if (StrFont)
	{
		auto it = _XmlContext.Fonts.find(StrFont);
		oCheck(it != _XmlContext.Fonts.end(), std::errc::invalid_argument, "Invalid or missing Font %s defined in Control %s", StrFont, StrID);
		_pDesc->font = (ouro::font_handle)it->second;
	}

	// Resolve size relative to parent/context
	int2 ControlPos(0, 0);
	if (!_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Pos", &ControlPos))
		oThrow(std::errc::invalid_argument, "Invalid or missing Pos attribute for Control %s", StrID);

	int2 ControlSize(oDEFAULT, oDEFAULT);
	_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Size", &ControlSize); // allow oDEFAULT to fall through if not specified
	ControlSize = oWinControlGetInitialSize(_pDesc->type, ControlSize);

	ouro::alignment alignment = ouro::alignment::top_left;
	_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Align", &alignment); // allow TopLeft as default value

	RECT rParentClient;
	int2 ParentSize = ouro::resolve_default(_ControlContext.ParentSize, oWinRectSize(rParentClient));
	RECT rParent = oWinRectWH(int2(0,0), ParentSize);

	RECT rControl = oWinRect(ouro::resolve_rect(oRect(rParent), ControlPos, ControlSize, alignment, false));
	_pDesc->position = oWinRectPosition(rControl);
	_pDesc->size = oWinRectSize(rControl);

	return true;
}

void oWinControlSet::AddControl(int _ID, HWND _hHandle)
{
	safe_set(Controls, _ID, _hHandle);
}

HWND oWinControlSet::CreateControl(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, controls_t* _pControls, int2 _ParentOffset)
{
	ouro::control_info d;
	if (!ParseControlDesc(_XmlContext, _ControlContext, *_pControls, &d))
		return nullptr; // pass through error

	bool enabled = _ControlContext.Enabled;
	bool visible = _ControlContext.Visible;
	_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Enabled", &enabled);
	_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Visible", &visible);

	d.position += _ParentOffset;
	HWND hControl = oWinControlCreate(d);
	oVB(hControl);

	// @tony: there still might be some redraw issues if this is the last control 
	// that may justify moving visible and enabled into control creation to take 
	// advantage of first-draw

	if (!enabled)
		oWinControlEnable(hControl, false);

	if (!visible)
		oWinControlSetVisible(hControl, false);

	safe_set(*_pControls, d.id, hControl);
	return hControl;
}

bool oWinControlSet::CreateControlsSibling(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, controls_t* _pControls)
{
	xml::node hChild = _XmlContext.pXML->first_child(_XmlContext.hNode);
	while (hChild)
	{
		XML_CONTEXT xc = _XmlContext;
		xc.hNode = hChild;
		if (!CreateControlsRecursive(xc, _ControlContext, _pControls))
			return false; // pass through error
		hChild = _XmlContext.pXML->next_sibling(hChild);
	}

	return true;
}

bool oWinControlSet::CreateControlsRecursive(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, controls_t* _pControls)
{
	CONTROL_CONTEXT Current = _ControlContext;
	const char* NodeName = _XmlContext.pXML->node_name(_XmlContext.hNode);
	if (oSTRVALID(NodeName))
	{
		bool VisitChildren = false;
		int2 RelativePos;
		int2 Size(-1, -1);
		if (!_stricmp("RefPoint", NodeName))
		{
			//#ifdef _DEBUG
			//	oTrace("<RefPoint Name=\"%s\" ... >", oSAFESTRN(_XmlContext.pXML->GetValue(_XmlContext.hNode, "Name")));
			//#endif
			  
			if (_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Pos", &RelativePos))
				Current.ParentPosition += RelativePos;

			if (_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Size", &Size))
				Current.ParentSize = Size; 

			const char* StrFont = _XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Font");
			if (StrFont)
				Current.hFont = ((fonts_t&)_XmlContext.Fonts)[StrFont];

			_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Visible", &Current.Visible);
			_XmlContext.pXML->find_attr_value(_XmlContext.hNode, "Enabled", &Current.Enabled);

			VisitChildren = true;
		}

		else if (!_stricmp("Control", NodeName))
		{
			Current.hParent = CreateControl(_XmlContext, Current, _pControls, Current.ParentPosition);
			if (!Current.hParent)
				return false; // pass through error

			RECT rParent = oWinGetRelativeRect(Current.hParent);
			Current.ParentPosition = oWinRectPosition(rParent);
			Current.ParentSize = oWinRectSize(rParent);
			VisitChildren = true;
		}

		if (VisitChildren)
		{
			if (!CreateControlsSibling(_XmlContext, Current, _pControls))
				return false; // pass through error
		}
	}
	return true;
}
