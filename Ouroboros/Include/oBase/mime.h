// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Internet media type:
// http://en.wikipedia.org/wiki/Mime_type

#pragma once

namespace ouro {

enum class mime
{
	// There is no unknown type in the standard, in a such a case a type name is 
	// not supposed to be sent. For example in the http protocol when an unknown 
	// MIME type is encountered just don't send the Content-Type header.
	unknown,

	application_atomxml,
	application_ecmascript,
	application_edix12,
	application_edifact,
	application_exe,
	application_json,
	application_javascript,
	application_octetstream,
	application_ogg,
	application_pdf,
	application_postscript,
	application_rdfxml,
	application_rssxml,
	application_soapxml,
	application_fontwoff,
	application_xdmp,
	application_xhtmlxml,
	application_xmldtd,
	application_xopxml,
	application_zip,
	application_gzip,
	application_7z,

	audio_basic,
	audio_l24,
	audio_mp4,
	audio_mpeg,
	audio_ogg,
	audio_vorbis,
	audio_wma,
	audio_wax,
	audio_realaudio,
	audio_wav,
	audio_webm,

	image_gif,
	image_jpeg,
	image_pjpeg,
	image_png,
	image_svgxml,
	image_tiff,
	image_ico,

	message_http,
	message_imdnxml,
	message_partial,
	message_rfc822,

	model_example,
	model_iges,
	model_mesh,
	model_vrml,
	model_x3dbinary,
	model_x3dvrml,
	model_x3dxml,

	multipart_mixed,
	multipart_alternative,
	multipart_related,
	multipart_formdata,
	multipart_signed,
	multipart_encrypted,

	text_cmd,
	text_css,
	text_csv,
	text_html,
	text_javascript,
	text_plain,
	text_vcard,
	text_xml,

	video_mpeg,
	video_mp4,
	video_ogg,
	video_quicktime,
	video_webm,
	video_matroska,
	video_wmv,
	video_flash_video,

	count,
};

// as_string, to_string, from_string for conversion to/from the "application/json" form

// from file extension to mime
mime from_extension(const char* ext);

}
