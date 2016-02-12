// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oCore/version.h>
#include <oMemory/allocate.h>
#include <oString/path.h>
#include <oGPU/gpu.h>

enum D3D_FEATURE_LEVEL;

namespace ouro { namespace gpu { namespace d3d {

// Create a set of command line options that you would pass to fxc and the 
// loaded source to compile to returns the compiled bytecode allocated from
// the specified allocator.
blob compile_shader(const char* cmdline_options, const path_t& shader_source_path, const char* shader_source, const allocator& alloc);

// Given a shader model (i.e. 4.0) return a feature level (i.e. D3D_FEATURE_LEVEL_10_1)
D3D_FEATURE_LEVEL feature_level(const version_t& shader_model);

// Returns the shader profile for the specified stage of the specified feature
// level. If the specified feature level does not support the specified stage,
// this will return nullptr.
const char* shader_profile(D3D_FEATURE_LEVEL level, const stage_binding::flag& stage_binding);

}}}
