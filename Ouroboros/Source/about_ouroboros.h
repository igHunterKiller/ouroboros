// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oArch/dependencies.h>
#include <oGUI/about.h>

#define oOURO_DEPENDENCY(lib_name, lib_version, url_home, url_license, desc) lib_name "|"
static const char* OUROBOROS_COMPONENTS = oOURO_DEPENDENCIES;
#undef oOURO_DEPENDENCY
#define oOURO_DEPENDENCY(lib_name, lib_version, url_home, url_license, desc) "<a href=\"" url_home "\">homepage</a>  <a href=\"" url_license "\">license</a>\r\n" lib_version "\r\n" desc "|"
static const char* OUROBOROS_COMPONENT_COMMENTS = oOURO_DEPENDENCIES;
#undef oOURO_DEPENDENCY

#define oDECLARE_ABOUT_INFO(_about_info_var, _icon) \
	ouro::about::info _about_info_var; \
	_about_info_var.icon               = (ouro::icon_handle)_icon; \
	_about_info_var.website            = oOURO_WEBSITE; \
	_about_info_var.issue_site         = oOURO_SUPPORT; \
	_about_info_var.components         = OUROBOROS_COMPONENTS; \
	_about_info_var.component_comments = OUROBOROS_COMPONENT_COMMENTS;
