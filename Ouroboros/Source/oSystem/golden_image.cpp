// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/golden_image.h>
#include <oSystem/filesystem.h>
#include <oSystem/system.h>
#include <oSurface/codec.h>

namespace ouro {

void golden_image::initialize(const init_t& init)
{
	struct ctx_t
	{
		const init_t* init;
		adapter::info_t adapter_info;
	};

	ctx_t ctx;
	ctx.init = &init;
	adapter::enumerate([](const adapter::info_t& info, void* user)->bool
	{
		auto& ctx = *(ctx_t*)user;

		if (info.id == ctx.init->adapter_id)
		{
			ctx.adapter_info = info;
			return false;
		}
		return true;
	}, &ctx);

	path_string tmp;

	golden_[root]   = init.golden_path_root;
	failed_[root]   = init.failed_path_root;
	golden_[os]     = golden_[root] / system::operating_system_name(tmp);
	failed_[os]     = failed_[root] / tmp;
	golden_[vendor] = golden_[os] / as_string(ctx.adapter_info.vendor);
	failed_[vendor] = failed_[os] / as_string(ctx.adapter_info.vendor);
	replace(tmp, ctx.adapter_info.description, " ", "_");
	golden_[hw]     = golden_[vendor] / tmp;
	failed_[hw]     = failed_[vendor] / tmp;
	golden_[driver] = golden_[hw] / to_string(tmp, ctx.adapter_info.version);
	failed_[driver] = failed_[hw] / tmp;
}

void golden_image::deinitialize()
{
}

void golden_image::test(const char* test_name, const surface::image& img, uint32_t nth_img, float max_rms_error, uint32_t diff_img_multiplier)
{
	char tmp[64];
	char filename[oMAX_PATH];
	snprintf(filename, "%s%s.png", test_name, nth_img == 0 ? "" : to_string(tmp, nth_img));
	const auto save_fmt = surface::get_file_format(filename);

	auto img_info = img.info();
	path_t failed_path;
	for (int i = driver; i >= root; i--)
	{
		path_t golden_path = golden_[i] / filename;
		failed_path = failed_[driver] / filename;
		if (filesystem::exists(golden_path))
		{
			// shorten long path names for simpler reporting
			const size_t cmn = cmnroot(golden_path, failed_path);
			const char* gpath = golden_path.c_str() + cmn;
			const char* fpath = failed_path.c_str() + cmn;

			// load the golden image
			surface::image golden_img;
			{
				blob b;
				try { b = filesystem::load(golden_path); }
				catch (std::exception&) { oThrow(std::errc::io_error, "Load failed: (Golden)...%s", gpath); }

				try { golden_img = surface::decode(b, img_info.format); }
				catch (std::exception&) { oThrow(std::errc::protocol_error, "Corrupt image: (Golden)...%s", gpath); }
			}

			auto golden_info = golden_img.info();

			// compare basics before diving into pixels
			{
				if (any(img_info.dimensions != golden_info.dimensions))
				{
					try
					{
						blob encoded = encode(img, save_fmt, img_info.format);
						filesystem::save(failed_path, encoded);
					}
					catch (std::exception&)
					{
						oThrow(std::errc::io_error, "Save failed: (Output)...%s", fpath);
					}
					
					oThrow(std::errc::protocol_error, "Dimension mismatch: (Output %dx%d)...%s != (Golden %dx%d)...%s"
						, img_info.dimensions.x, img_info.dimensions.y, fpath
						, golden_info.dimensions.x, golden_info.dimensions.y, gpath);
				}

				if (img_info.format != golden_info.format)
				{
					try
					{
						blob encoded = encode(img, save_fmt, img_info.format);
						filesystem::save(failed_path, encoded);
					}
					catch (std::exception&)
					{
						oThrow(std::errc::io_error, "Save failed: (Output)...", fpath);
					}
					
					oThrow(std::errc::protocol_error, "Format mismatch: (Output %s)...%s != (Golden %s)...%s", as_string(img_info.format), fpath, as_string(golden_info.format), gpath);
				}
			}

			// test pixels
			surface::image diff_img;
			float rms_err = surface::calc_rms(img, golden_img, &diff_img, diff_img_multiplier);

			// save out test image and diffs if there is a non-similar result
			if (rms_err > max_rms_error)
			{
				// save failed image
				try
				{
					blob encoded = encode(img, save_fmt, img_info.format);
					filesystem::save(failed_path, encoded);
				}
				catch (std::exception&) { oThrow(std::errc::io_error, "Save failed: (Output)...%s", fpath); }

				// save diff image
				path_t diff_path(failed_path);
				diff_path.replace_extension_with_suffix("_diff.png");
				const char* dpath = diff_path.c_str() + cmn;

				try
				{
					blob encoded = encode(diff_img, save_fmt, surface::format::r8_unorm);
					filesystem::save(diff_path, encoded);
				}
				catch (std::exception&) { oThrow(std::errc::io_error, "Save failed: (Diff)...%s", dpath); }

				oThrow(std::errc::protocol_error, "Compare failed: %.03f RMS error (threshold %.03f): (Output)...%s != (Golden)...%s", rms_err, max_rms_error, fpath, gpath);
			}

			return;
		}
	}

	// golden image not found, save one
	try
	{
		blob encoded = encode(img, save_fmt, img_info.format);
		filesystem::save(failed_path, encoded);
	}
	catch (std::exception&) { oThrow(std::errc::io_error, "Save failed: (Failed)", failed_path.c_str()); }

	oThrow(std::errc::no_such_file_or_directory, "Not found: (Golden).../%s result image saved to %s", filename, failed_path.c_str());
}

}