// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "d3d_include.h"
#include <oSystem/filesystem.h>

namespace ouro { namespace gpu { namespace d3d {

HRESULT include::Close(LPCVOID pData)
{
	//free((void*)pData); // don't destroy cached files
	return S_OK;
}

HRESULT include::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, uint32_t* pBytes)
{
	try
	{
		path_t filename(pFileName);

		blob* c = nullptr;
		if (cache_.get_ptr(filename.hash(), &c))
		{
			*ppData = *c;
			*pBytes = (uint32_t)c->size();
			return S_OK;
		}

		path_t full_path(filename);
		bool exists = filesystem::exists(full_path);
		if (!exists)
		{
			for (const path_t& p : search_paths_)
			{
				full_path = p / filename;
				exists = filesystem::exists(full_path);
				if (exists)
					break;
			}
		}

		if (!exists)
		{
			std::string err(filename);
			err += " not found in search path: ";
			if (search_paths_.empty())
				err += "<empty>";
			else
				for (const path_t& p : search_paths_)
				{
					err += p.c_str();
					err += ";";
				}
			
			throw std::system_error(std::errc::no_such_file_or_directory, std::system_category(), err.c_str());
		}

		blob source = filesystem::load(full_path);

		*ppData = source;
		*pBytes = (uint32_t)source.size();
		if (!cache_.add(filename.hash(), std::move(source)))
			throw std::system_error(std::errc::no_buffer_space, std::system_category(), "add failed");
	}

	catch (std::exception&)
	{
		*ppData = nullptr;
		*pBytes = 0;
		return E_FAIL;
	}

	return S_OK;
}

}}}
