// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oBase/growable_hash_map.h>
#include <oString/path.h>
#include <d3d11.h>
#include <atomic>

namespace ouro { namespace gpu { namespace d3d {

class include : public ID3DInclude
{
public:
	enum flags
	{
		respect_refcount = 1, // if specified Release() will call delete this, else Release() will do nothing (for stack-allocated objects)
	};

	include(const path_t& shader_source_path, int flags = 0)
		: shader_source_path_(shader_source_path)
		, cache_(50)
		, flags_(flags)
		, refcount_(1)
	{}

	ULONG AddRef() { return ++refcount_; }
	ULONG Release() { ULONG r = --refcount_; if (!r && (flags_ & respect_refcount)) delete this; return r; }
	IFACEMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) { return ppvObject ? E_NOINTERFACE : E_POINTER; }
	IFACEMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, uint32_t *pBytes);
	IFACEMETHOD(Close)(THIS_ LPCVOID pData);

	void add_search_path(const path_t& _SearchPath) { search_paths_.push_back(_SearchPath); }
	void clear_search_paths() { search_paths_.clear(); }
	void clear_cached_files() { cache_.clear(); }

protected:
	path_t shader_source_path_;
	std::vector<path_t> search_paths_;
	growable_hash_map<uint64_t, blob> cache_;
	int flags_;
	std::atomic<int> refcount_;
};

}}}
