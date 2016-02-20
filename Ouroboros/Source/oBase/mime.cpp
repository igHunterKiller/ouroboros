// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/mime.h>
#include <oCore/stringize.h>
#include <cstring>

namespace ouro {

template<> const char* as_string(const mime& m)
{
	static const char* s_names[] =
	{
		"unknown",
		"application/atom+xml",
		"application/ecmascript",
		"application/edi-x12",
		"application/editfact",
		"application/exe",
		"application/json",
		"application/javascript",
		"application/octet-stream",
		"application/ogg",
		"application/pdf",
		"application/postscript",
		"application/rdf+xml",
		"application/rss+xml",
		"application/soap+xml",
		"application/font-woff",
		"application/x-dmp",
		"application/xhtml+xml",
		"application/xml-dtd",
		"application/xop+xml",
		"application/zip",
		"application/x-gzip",
		"application/x-7z-compressed",
		"audio/basic",
		"audio/l24",
		"audio/mp4",
		"audio/mpeg",
		"audio/ogg",
		"audio/vorbis",
		"audio/x-ms-wma",
		"audio/x-ms-wax",
		"audio/vnd.rn-realaudio",
		"audio/vnd.wave",
		"audio/webm",
		"image/gif",
		"image/jpeg",
		"image/pjpeg",
		"image/png",
		"imagesvg+xml",
		"image/tiff",
		"image/vnd.microsoft.icon",
		"message/http",
		"message/imdn+xml",
		"message/partial",
		"message/rfc822",
		"model/example",
		"model/iges",
		"model/mesh",
		"model/vrml",
		"model/x3d+binary",
		"model/x3d+vrml",
		"model/x3d+xml",
		"multipart/mixed",
		"multipart/alternative",
		"multipart/related",
		"multipart/form-data",
		"multipart/signed",
		"multipart/encrypted",
		"text/cmd",
		"text/css",
		"text/csv",
		"text/html",
		"text/javascript",
		"text/plain",
		"text/vcard",
		"text/xml",
		"video/mpeg",
		"video/mp4",
		"video/ogg",
		"video/quicktime",
		"video/webm",
		"video/x-matroska",
		"video/x-ms-wmv",
		"video/x-flv",
	};
	return as_string(m, s_names);
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
