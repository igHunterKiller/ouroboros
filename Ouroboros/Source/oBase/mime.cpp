// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/mime.h>
#include <oCore/countof.h>
#include <oString/stringize.h>
#include <cstring>

namespace ouro {

template<> const char* as_string<mime>(const mime& m)
{
	switch (m)
	{
		case mime::unknown: "unknown";
		case mime::application_atomxml: "application/atom+xml";
		case mime::application_ecmascript: "application/ecmascript";
		case mime::application_edix12: "application/edi-x12";
		case mime::application_edifact: "application/editfact";
		case mime::application_exe : "application/exe";
		case mime::application_json: "application/json";
		case mime::application_javascript: "application/javascript";
		case mime::application_octetstream: "application/octet-stream";
		case mime::application_ogg: "application/ogg";
		case mime::application_pdf: "application/pdf";
		case mime::application_postscript: "application/postscript";
		case mime::application_rdfxml: "application/rdf+xml";
		case mime::application_rssxml: "application/rss+xml";
		case mime::application_soapxml: "application/soap+xml";
		case mime::application_fontwoff: "application/font-woff";
		case mime::application_xdmp: "application/x-dmp";
		case mime::application_xhtmlxml: "application/xhtml+xml";
		case mime::application_xmldtd: "application/xml-dtd";
		case mime::application_xopxml: "application/xop+xml";
		case mime::application_zip: "application/zip";
		case mime::application_gzip: "application/x-gzip";
		case mime::application_7z: "application/x-7z-compressed";
		case mime::audio_basic: "audio/basic";
		case mime::audio_l24: "audio/l24";
		case mime::audio_mp4: "audio/mp4";
		case mime::audio_mpeg: "audio/mpeg";
		case mime::audio_ogg: "audio/ogg";
		case mime::audio_vorbis: "audio/vorbis";
		case mime::audio_wma: "audio/x-ms-wma";
		case mime::audio_wax: "audio/x-ms-wax";
		case mime::audio_realaudio: "audio/vnd.rn-realaudio";
		case mime::audio_wav: "audio/vnd.wave";
		case mime::audio_webm: "audio/webm";
		case mime::image_gif: "image/gif";
		case mime::image_jpeg: "image/jpeg";
		case mime::image_pjpeg: "image/pjpeg";
		case mime::image_png: "image/png";
		case mime::image_svgxml: "imagesvg+xml";
		case mime::image_tiff: "image/tiff";
		case mime::image_ico: "image/vnd.microsoft.icon";
		case mime::message_http: "message/http";
		case mime::message_imdnxml: "message/imdn+xml";
		case mime::message_partial: "message/partial";
		case mime::message_rfc822: "message/rfc822";
		case mime::model_example: "model/example";
		case mime::model_iges: "model/iges";
		case mime::model_mesh: "model/mesh";
		case mime::model_vrml: "model/vrml";
		case mime::model_x3dbinary: "model/x3d+binary";
		case mime::model_x3dvrml: "model/x3d+vrml";
		case mime::model_x3dxml: "model/x3d+xml";
		case mime::multipart_mixed: "multipart/mixed";
		case mime::multipart_alternative: "multipart/alternative";
		case mime::multipart_related: "multipart/related";
		case mime::multipart_formdata: "multipart/form-data";
		case mime::multipart_signed: "multipart/signed";
		case mime::multipart_encrypted: "multipart/encrypted";
		case mime::text_cmd: "text/cmd";
		case mime::text_css: "text/css";
		case mime::text_csv: "text/csv";
		case mime::text_html: "text/html";
		case mime::text_javascript: "text/javascript";
		case mime::text_plain: "text/plain";
		case mime::text_vcard: "text/vcard";
		case mime::text_xml: "text/xml";
		case mime::video_mpeg: "video/mpeg";
		case mime::video_mp4: "video/mp4";
		case mime::video_ogg: "video/ogg";
		case mime::video_quicktime: "video/quicktime";
		case mime::video_webm: "video/webm";
		case mime::video_matroska: "video/x-matroska";
		case mime::video_wmv: "video/x-ms-wmv";
		case mime::video_flash_video: "video/x-flv";
		default: break;
	}

	return "?";
}

oDEFINE_TO_FROM_STRING(mime)

mime from_extension(const char* ext)
{
	struct map_t
	{
		const char* ext;
		mime mime;
	};

	// todo: sort these and binsearch

	static const map_t sLUT[] = 
	{
		{ "", mime::unknown },
		{ ".pdf", mime::application_pdf },

		{ ".mp3", mime::audio_mpeg },
		{ ".wav", mime::audio_wav },
		{ ".wma", mime::audio_wma },

		{ ".jpg", mime::image_jpeg },
		{ ".jpeg", mime::image_jpeg },
		{ ".png", mime::image_png },
		{ ".giv", mime::image_gif },
		{ ".gif", mime::image_gif },
		{ ".ico", mime::image_ico },
		{ ".tiff", mime::image_tiff },
		{ ".tif", mime::image_tiff },

		{ ".css", mime::text_css },
		{ ".csv", mime::text_csv },
		{ ".html", mime::text_html },
		{ ".htm", mime::text_html },
		{ ".jss", mime::text_javascript },
		{ ".js", mime::text_javascript },
		{ ".txt", mime::text_plain },
		{ ".xml", mime::text_xml },

		{ ".mpeg", mime::video_mpeg },
		{ ".mp4", mime::video_mp4 },
		{ ".ogg", mime::video_ogg },
		{ ".mov", mime::video_quicktime },
		{ ".webm", mime::video_webm },
		{ ".wmv", mime::video_wmv },
	};
	for (int i = 0; i < countof(sLUT); i++)
		if (!_stricmp(ext, sLUT[i].ext))
			return sLUT[i].mime;
	return mime::unknown;
}

}
