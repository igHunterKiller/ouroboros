// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Parser that can turn XML into an array of HWND controls and access them once
// created. NOTE: This only creates the controls, it does not link them to doing
// anything useful - client code will still be necessary. An extensive example 
// file can be found alongside TESTWindowSysProps, but for the basics:

/**

==== Version 1.0 ====

There are three node types: Control, RefPoint, Font
=== Font === 
Font defines a font and identifies it by name to be used in Control and 
RefPoint nodes, so it makes sense to define all Fonts up front. It is based 
on ouro::font_info: the field names are the same plus one more as the name of 
the font definition.
* FontName: (i.e. Tahoma) is the string name of the typeface as it appears in 
  the operating system. 
* PointSize: the floating point size is that number in other GUIs used to 
  indicate the size of the font. NOTE: This may get rounded to an integer.
* Bold|Italic|Underline|StrikeOut: Bool true/false's that modify the font.
* Name: This is the name by which RefPoints and Controls can refer to this 
  definition.

=== RefPoint ===
Sometimes it is useful to group several controls together with common 
properties. A RefPoint serves that purpose. Non-defined fields can be used (such 
as "Name" to document the group of controls. The following properties if defined 
in a RefPoint will apply to its children RefPoints and Controls:
* Font: The string name of a pre-defined font.
* Pos: Sets a new int2 position from which all child positions are offsets.
* Size: When children ask for their Parent's size (for alignment mostly) this 
  value will be given.
* Visible: Boolean indicating that upon load all child Controls are set with 
  this visibility.
* Enable: Boolean indicating that upon load all child Controls are set with this
  visibility.

=== Control ===
This is the actual object being created. A Control node populates an 
ouro::control_info, with a few additional initial-state fields. If a field isn't 
explicitly assigned a value, either a system default or inherited value will be 
used. Both RefPoints and other Controls can be parents. A Control's underlying 
system parent will be the last Control up the hierarchy or the top-level window.
The fields are:
* Font: The string name of the font to use as previously defined with a Font 
  node.
* Type: This must be specified as a string. Valid types are:
  Group Button Checkbox Radio Label Hyperlabel SelectableLabel Icon Textbox 
  Floatbox FloatboxSpinner Combobox ComboTextbox ProgressBar ProgressBarUnknown 
  Tab Silder SelectableSlider
* Text: The string to display. NOTE: a '|'-terminated set of strings can be 
  specified immediately populate all subitems ("Item1|Item2|Item3") if the 
	control supports subitems
* Align: Specifies where the control will be created relative to its parent. 
  Valid values are: TopLeft TopCenter TopRight MiddleLeft MiddleCenter 
	MiddleRight BottomLeft BottomCetner BottomRight
* Position: int2 describing where the position is relative to its parent and 
  alignment.
* Size: int2 describing how large the control is
* ID: A string representing the logical ID used for events and actions when this
  control is used. In code ouro::from_string is used to link this to a real code enum
	and if not, an error will be broadcast. Minimize the need to compile be 
	designing functionality and general layout first, enumerating all the controls
	to be defined, compiling that, and positioning and tweaking with the XML.
* StartsNewGroup: Boolean as to whether this object starts a new group. A group 
  is the set of controls in which radio button behavior (only one selected at a 
	time) is enforced.

An example XML:

<?xml version="1.0" encoding="ISO-8859-1" ?>
<oUI version="1.0">
	<Font Name="MyFont" FontName="Tahoma" PointSize="12" Underline="true" />
	<RefPoint Name="SampleControls" Pos="5 5" Font="MyFont">
		<Control Type="Group" Text="Sample" Pos="10 10" Size="150 120" ID="ID_GROUP">
			<Control Type="Hyperlable" Text="&lt;a id=&quot;SOME_ID&quot;&gt;Click me&lt;/a&gt;" Pos="10 10" Size="100 20" ID="ID_LINK" />
		</Control>
	</RefPoint>
	<Control Type="Button" Text="&OK" Align="BottomRight" Pos="-10 -10" ID="ID_OK" />
</oUI>

*/

#pragma once
#ifndef oWinControlSet_h
#define oWinControlSet_h

#include <oGUI/oGUI.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oBase/container_support.h>
#include <oString/fixed_string.h>
#include <oString/xml.h>
#include <map>
#include <string>
#include <vector>

class oWinControlSet
{
public:
	typedef std::function<bool(int* _pID, const char* _StrSource)> IDFROMSTRING;

	// Respect ParentClientSize since Initialize might be called from a WM_CREATE 
	// message before the window is actually properly sized.
	void Initialize(HWND _hParent, const int2& _ParentClientSize, const ouro::xml& _XML, const IDFROMSTRING& _IDFromString);
	void Deinitialize();

	HWND GetControl(int _ID) const;
	void AddControl(int _ID, HWND _hHandle);

	inline HWND operator[](int _Index) const { return GetControl(_Index); }

private:
	typedef std::map<ouro::mstring, HFONT, ouro::less_i<ouro::mstring>> fonts_t;

	struct CONTROL_CONTEXT
	{
		CONTROL_CONTEXT()
			: hParent(nullptr)
			, hFont(nullptr)
			, ParentPosition(0,0)
			, ParentSize(oDEFAULT, oDEFAULT)
			, Visible(true)
			, Enabled(true)
		{}

		HWND hParent;
		HFONT hFont;
		int2 ParentPosition;
		int2 ParentSize;
		IDFROMSTRING IDFromString;
		bool Visible;
		bool Enabled;
	};

	struct XML_CONTEXT
	{
		XML_CONTEXT(const fonts_t& _Fonts)
			: Fonts(_Fonts)
		{}

		const ouro::xml* pXML;
		ouro::xml::node hNode;
		const fonts_t& Fonts;
	};

	typedef std::vector<HWND> controls_t;
	controls_t Controls;

	bool ParseFontDesc(const ouro::xml& _XML, ouro::xml::node _hNode, ouro::font_info* _pDesc);
	void CreateFontsSibling(const ouro::xml& _XML, fonts_t* _pFonts);
	bool ParseControlDesc(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, const controls_t& _Controls, ouro::control_info* _pDesc);
	HWND CreateControl(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, controls_t* _pControls, int2 _ParentOffset);
	bool CreateControlsRecursive(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, controls_t* _pControls);
	bool CreateControlsSibling(const XML_CONTEXT& _XmlContext, const CONTROL_CONTEXT& _ControlContext, controls_t* _pControls);
};

#endif
