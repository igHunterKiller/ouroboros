// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/byte.h>
#include <oCore/finally.h>
#include <oCore/stringf.h>
#include <oString/argtok.h>
#include <oSystem/windows/win_util.h>
#include "d3d_compile.h"
#include "d3d_include.h"
#include <d3d11.h>
#include <D3Dcompiler.h>

namespace ouro { namespace gpu { namespace d3d {

// Replace all occurrences of find in str with replace. This returns the 
// number of replacements made.
template<typename T, typename traitsT, typename allocatorT>
static size_t replace_all(std::basic_string<T, traitsT, allocatorT>& str, const char* find, const char* replace)
{
	size_t n = 0;
	if ( find && *find && replace && *replace)
	{
		std::basic_string<T, traitsT, allocatorT>::size_type find_len = strlen(find);
		std::basic_string<T, traitsT, allocatorT>::size_type replace_len = strlen(replace);
		std::basic_string<T, traitsT, allocatorT>::size_type pos = str.find(find, 0);
		for (; pos != std::string::npos; pos += replace_len)
		{
			str.replace(pos, find_len, replace);
			pos = str.find(find, 0);
			n++;
		}
	}
	return n;
}

static void d3dcompile_convert_error_buffer(char* _OutErrorMessageString, size_t _SizeofOutErrorMessageString, ID3DBlob* _pErrorMessages, const char** _pIncludePaths, size_t _NumIncludePaths)
{
	const char* msg = (const char*)_pErrorMessages->GetBufferPointer();

	if (!_OutErrorMessageString)
		throw std::invalid_argument("");

	if (_pErrorMessages)
	{
		std::string tmp;
		tmp.reserve(10 * 1024);
		tmp.assign(msg);
		replace_all(tmp, "%", "%%");

		// Now make sure header errors include their full paths
		if (_pIncludePaths && _NumIncludePaths)
		{
			size_t posShortHeaderEnd = tmp.find(".h(");
			while (posShortHeaderEnd != std::string::npos)
			{
				size_t posShortHeader = tmp.find_last_of("\n", posShortHeaderEnd);
				if (posShortHeader == std::string::npos)
					posShortHeader = 0;
				else
					posShortHeader++;

				posShortHeaderEnd += 2; // absorb ".h" from search

				size_t shortHeaderLen = posShortHeaderEnd - posShortHeader;

				std::string shortPath;
				shortPath.assign(tmp, posShortHeader, shortHeaderLen);
				std::string path;
				path.reserve(1024);
				
				for (size_t i = 0; i < _NumIncludePaths; i++)
				{
					path.assign(_pIncludePaths[i]);
					path.append("/");
					path.append(shortPath);

					if (filesystem::exists(path.c_str()))
					{
						tmp.replace(posShortHeader, shortHeaderLen, path.c_str());
						posShortHeaderEnd = tmp.find("\n", posShortHeaderEnd); // move to end of line
						break;
					}
				}

				posShortHeaderEnd = tmp.find(".h(", posShortHeaderEnd);
			}
		}

		const char* start = tmp.c_str();
		const char* TruncatedPath = strstr(start, "?????");
		if (TruncatedPath)
			start = TruncatedPath + 5;

		strlcpy(_OutErrorMessageString, start, _SizeofOutErrorMessageString);
	}

	else
		*_OutErrorMessageString = 0;
}

blob compile_shader(const char* _CommandLineOptions, const path_t& _ShaderSourceFilePath, const char* _ShaderSource, const allocator& _Allocator)
{
	int argc = 0;
	const char** argv = argtok(malloc, nullptr, _CommandLineOptions, &argc);
	oFinally { free(argv); };

	std::string UnsupportedOptions("Unsupported options: ");
	size_t UnsupportedOptionsEmptyLen = UnsupportedOptions.size();

	const char* TargetProfile = "";
	const char* EntryPoint = "main";
	std::vector<const char*> IncludePaths;
	std::vector<std::pair<std::string, std::string>> Defines;
	unsigned int Flags1 = 0, Flags2 = 0;

	for (int i = 0; i < argc; i++)
	{
		const char* inc = nullptr;
		const char* sw = argv[i];
		const int o = to_upper(*(sw+1));
		const int o2 = to_upper(*(sw+2));
		const int o3 = to_upper(*(sw+3));

		std::string StrSw(sw);

		#define TRIML(str) ((str) + strspn(str, oWHITESPACE))

		if (*sw == '/')
		{
			switch (o)
			{
				case 'T':
					TargetProfile = TRIML(sw+2);
					if (!*TargetProfile)
						TargetProfile = argv[i+1];
					break;
				case 'E':
					EntryPoint = TRIML(sw+2);
					if (!*EntryPoint)
						EntryPoint = argv[i+1];
					break;
				case 'I':
					inc = TRIML(sw+2);
					if (!*inc)
						inc = argv[i+1];
					IncludePaths.push_back(inc);
					break;
				case 'O':
				{
					switch (o2)
					{
						case 'D':	Flags1 |= D3DCOMPILE_SKIP_OPTIMIZATION; break;
						case 'P':	Flags1 |= D3DCOMPILE_NO_PRESHADER; break;
						case '0': Flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL0; break;
						case '1': Flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL1; break;
						case '2': Flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL2; break;
						case '3': Flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL3; break;
						default: UnsupportedOptions.append(" " + StrSw); break;
					}

					break;
				}
				case 'W':
				{
					if (o2 == 'X') Flags1 |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
					else UnsupportedOptions.append(" " + StrSw);
					break;
				}
				case 'V':
				{
					if (o2 == 'D') Flags1 |= D3DCOMPILE_SKIP_VALIDATION;
					else UnsupportedOptions.append(" " + StrSw);
					break;
				}
				case 'Z':
				{
					switch (o2)
					{
						case 'I':
							Flags1 |= D3DCOMPILE_DEBUG;
							break;
						case 'P':
						{
							switch (o3)
							{
								case 'R': Flags1 |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR; break;
								case 'C': Flags1 |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR; break;
								default: UnsupportedOptions.append(" " + StrSw); break;
							}

							break;
						}
						default: UnsupportedOptions.append(" " + StrSw); break;
					}
						
					break;
				}
				case 'G':
				{
					if (o2 == 'P' && o3 == 'P') Flags1 |= D3DCOMPILE_PARTIAL_PRECISION;
					else if (o2 == 'F' && o3 == 'A') Flags1 |= D3DCOMPILE_AVOID_FLOW_CONTROL;
					else if (o2 == 'F' && o3 == 'P') Flags1 |= D3DCOMPILE_PREFER_FLOW_CONTROL;
					else if (o2 == 'D' && o3 == 'P') Flags2 |= D3D10_EFFECT_COMPILE_ALLOW_SLOW_OPS;
					else if (o2 == 'E' && o3 == 'S') Flags1 |= D3DCOMPILE_ENABLE_STRICTNESS;
					else if (o2 == 'E' && o3 == 'C') Flags1 |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
					else if (o2 == 'I' && o3 == 'S') Flags1 |= D3DCOMPILE_IEEE_STRICTNESS;
					else if (o2 == 'C' && o3 == 'H') Flags2 |= D3D10_EFFECT_COMPILE_CHILD_EFFECT;
					else UnsupportedOptions.append(" " + StrSw);
					break;
				}
				case 'D':
				{
					const char* k = TRIML(sw+2);
					if (!*k)
						k = argv[i+1];
					const char* sep = strchr(k, '=');
					const char* v = "1";
					if (sep)
						v = TRIML(v);
					else
						sep = k + strlen(k);

					Defines.resize(Defines.size() + 1);
					Defines.back().first.assign(k, sep-k);
					Defines.back().second.assign(v);
					break;
				}
				
				default: UnsupportedOptions.append(" " + StrSw); break;
			}
		}
	}

	if (UnsupportedOptionsEmptyLen != UnsupportedOptions.size())
		throw std::invalid_argument(UnsupportedOptions.c_str());

	std::vector<D3D_SHADER_MACRO> Macros;
	Macros.resize(Defines.size() + 1);
	for (size_t i = 0; i < Defines.size(); i++)
	{
		Macros[i].Name = Defines[i].first.c_str();
		Macros[i].Definition = Defines[i].second.c_str();
	}

	Macros.back().Name = nullptr;
	Macros.back().Definition = nullptr;

	char SourceName[oMAX_PATH];
	snprintf(SourceName, "%s", _ShaderSourceFilePath);

	include D3DInclude(_ShaderSourceFilePath);
	for (const path_t& p : IncludePaths)
		D3DInclude.add_search_path(p);

	ref<ID3DBlob> Code, Errors;
	HRESULT hr = D3DCompile(_ShaderSource
		, strlen(_ShaderSource)
		, SourceName
		, Macros.data()
		, &D3DInclude
		, EntryPoint
		, TargetProfile
		, Flags1
		, Flags2
		, &Code
		, &Errors);

	if (FAILED(hr))
	{
		size_t size = Errors->GetBufferSize() + 1 + 10 * 1024; // conversion can expand buffer, but not by very much, so pad a lot and hope expansion stays small

		std::unique_ptr<char[]> Errs(new char[size]);
		d3dcompile_convert_error_buffer(Errs.get(), size, Errors, IncludePaths.data(), IncludePaths.size());
		throw std::system_error(std::errc::io_error, std::system_category(), std::string("shader compilation error:\n") + Errs.get());
	}

	void* buffer = _Allocator.allocate(Code->GetBufferSize(), "compile_shader");
	memcpy(buffer, Code->GetBufferPointer(), Code->GetBufferSize());

	return blob(buffer, Code->GetBufferSize(), _Allocator.deallocator());
}

D3D_FEATURE_LEVEL feature_level(const version_t& shader_model)
{
	D3D_FEATURE_LEVEL                        level = D3D_FEATURE_LEVEL_9_1;
	     if (shader_model == version_t(3,0)) level = D3D_FEATURE_LEVEL_9_3;
	else if (shader_model == version_t(4,0)) level = D3D_FEATURE_LEVEL_10_0;
	else if (shader_model == version_t(4,1)) level = D3D_FEATURE_LEVEL_10_1;
	else if (shader_model == version_t(5,0)) level = D3D_FEATURE_LEVEL_11_0;
	return level;
}

const char* shader_profile(D3D_FEATURE_LEVEL level, const stage_binding::flag& stage_binding)
{
	static const char* sDX9Profiles[]    = { "vs_3_0", nullptr,  nullptr,  nullptr,  "ps_3_0", nullptr,  };
	static const char* sDX10Profiles[]   = { "vs_4_0", nullptr,  nullptr,  "gs_4_0", "ps_4_0", nullptr,  };
	static const char* sDX10_1Profiles[] = { "vs_4_1", nullptr,  nullptr,  "gs_4_1", "ps_4_1", nullptr,  };
	static const char* sDX11Profiles[]   = { "vs_5_0", "hs_5_0", "ds_5_0", "gs_5_0", "ps_5_0", "cs_5_0", };

	const char** profiles = 0;
	switch (level)
	{
		case D3D_FEATURE_LEVEL_9_1: case D3D_FEATURE_LEVEL_9_2: case D3D_FEATURE_LEVEL_9_3: profiles = sDX9Profiles; break;
		case D3D_FEATURE_LEVEL_10_0: profiles = sDX10Profiles; break;
		case D3D_FEATURE_LEVEL_10_1: profiles = sDX10_1Profiles; break;
		case D3D_FEATURE_LEVEL_11_0: profiles = sDX11Profiles; break;
		default: throw std::invalid_argument(stringf("unexpected D3D_FEATURE_LEVEL %d", level));
	}

	// go from mask to integer... 1? 2?

	const char* profile = profiles[log2i(stage_binding)];
	if (!profile)
	{
		version_t ver = version_t((level>>12) & 0xffff, (level>>8) & 0xffff);
		char str_ver[64];
		throw std::system_error(std::errc::not_supported, std::system_category(), std::string("Shader profile does not exist for D3D") + to_string(str_ver, ver) + "'s stage " + as_string(stage_binding));
	}

	return profile;
}

}}}
