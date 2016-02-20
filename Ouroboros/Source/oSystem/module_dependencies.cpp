// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This source is separate because <dbghelp.h> and <imagehlp.h> seem to define conflicting symbols.

#include <oCore/finally.h>
#include <oCore/hash_map.h>
#include <oSystem/module.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma warning(disable:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared
#include <Imagehlp.h>
#pragma warning(default:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared

namespace ouro { namespace module {

template <class T> static void* peimage_get_pointer(void* image_base, T* nt_header, DWORD offset)
{
	// Look through all the sections for the header
	IMAGE_SECTION_HEADER* header = nullptr;
	{
		IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt_header);
		const uint32_t nsections = nt_header->FileHeader.NumberOfSections;
		IMAGE_SECTION_HEADER* end = section + nsections;
		while (section < end)
		{
			const uint32_t start = section->VirtualAddress;
			const uint32_t finish = start + section->Misc.VirtualSize;

			if (offset >= start && offset < finish)
			{
				header = section;
				break;
			}

			section++;
		}
	}

	if (!header)
		return nullptr;
	
	const int diff = header->VirtualAddress - header->PointerToRawData;
	
	return (void*)((char*)image_base + offset - diff);
}

static void enum_dependencies_internal(const char* parent_path
	, path_t* out_dependency_paths, uint32_t* inout_num_dependency_paths, uint32_t max_num_dependency_paths
	, hash_map<uint64_t, uint8_t>& visited)
{
	// ideally 'visited' would be a hash_set with no value
	static const uint8_t kDummyValue = 1;

	const uint64_t parent_key = fnv1ai<uint64_t>(parent_path);

	// do not retraverse branches of the dependency graph that have already been visited.
	if (visited.exists(parent_key))
		return;

	// mark visited prior to any work because to prevent any circular dependency walking
	if (!visited.add(parent_key, kDummyValue))
		throw std::overflow_error("visited hash overflowed capacity");

	// do not enumerate dlls in the system directory because that adds scores of dlls that
	// are safe to assume on other Windows installations and should not be part of any 
	// copy or packaging of an exe and its dependencies.
	static const char* kSystemPrefix = "C:\\Windows";
	const size_t strlen_system_prefix = strlen(kSystemPrefix);
	
	bool system_module = false;
	int ndependencies = 0;
	LOADED_IMAGE* image = ImageLoad(parent_path, nullptr);
	if (image && image->FileHeader->OptionalHeader.NumberOfRvaAndSizes >= 2)
	{
		oFinally { if (image) ImageUnload(image); };

		system_module = !_strnicmp(kSystemPrefix, image->ModuleName, strlen_system_prefix);

		// skip system modules
		if (!system_module)
		{
			// append this module as a dependency using its true path

			if (*inout_num_dependency_paths >= max_num_dependency_paths)
				throw std::overflow_error("max num dependency path strings reached");

			out_dependency_paths[(*inout_num_dependency_paths)++] = image->ModuleName;

			// recurse through sub-dependencies
			IMAGE_IMPORT_DESCRIPTOR* import_desc = (IMAGE_IMPORT_DESCRIPTOR*)peimage_get_pointer(image->MappedAddress, image->FileHeader, image->FileHeader->OptionalHeader.DataDirectory[1].VirtualAddress);
			while (import_desc && (import_desc->TimeDateStamp || import_desc->Name))
			{
				const char* import_name = (const char*)peimage_get_pointer(image->MappedAddress, image->FileHeader, import_desc->Name);

				// skip ubiquitous system dependencies
				if (0 != _strnicmp(import_name, "api-ms-win-", 11))
					enum_dependencies_internal(import_name, out_dependency_paths, inout_num_dependency_paths, max_num_dependency_paths, visited);

				import_desc++;
			}
		}
	}
}

uint32_t enum_dependencies(const char* module_path, path_t* out_paths, uint32_t max_num_paths)
{
	oCheck(max_num_paths <= 512, std::errc::invalid_argument, "code uses stack memory for a hash set and %u possible entries is probably too large for that", max_num_paths);

	// set up a hash set to record which dependencies have already been traversed
	hash_map<uint64_t, uint8_t> visited; // no hash_set handy, so use hash_map

	const uint32_t visited_bytes = visited.calc_size(max_num_paths);
	void* visited_mem = alloca(visited_bytes);
	visited.initialize(visited_mem, visited_bytes);

	uint32_t npaths = 0;
	enum_dependencies_internal(module_path, out_paths, &npaths, max_num_paths, visited);

	return npaths;
}

}}
