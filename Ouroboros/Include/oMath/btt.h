// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// BTT (Binary Triangle Tree) as used for a square geometry patch designed
// so the patch can be instanced as neighbors to further cover a plane at
// varying tessellation levels and have each patch avoid T-junctions by
// defining various "link" forms that match level + 1 of its neighbors.

// Level 0 is a 2-tri quad. Level 1 is the X quad (0 below) and it and its links continue thus:

// +-----+  +-----+  +-----+  +--+--+  +-----+  +-----+  +-----+  +-----+  +-----+  +--+--+  +--+--+  +--+--+  +--+--+  +--+--+  +--+--+  +--+--+  +--+--+
// |\    |  |\   /|  |\   /|  |\   /|  |\   /|  |\   /|  |\   /|  |\   /|  |\   /|  |\ | /|  |\ | /|  |\ | /|  |\ | /|  |\ | /|  |\ | /|  |\ | /|  |\ | /|
// | \   |  | \ / |  | \ / |  | \ / |  | \ / |  | \ / |  | \ / |  | \ / |  | \ / |  | \|/ |  | \|/ |  | \|/ |  | \|/ |  | \|/ |  | \|/ |  | \|/ |  | \|/ |
// |  \  |  |  X  |  +--X  |  |  X  |  +--X  |  |  X--+  +--X--+  |  X--+  +--X--+  |  X  |  +--X  |  |  X  |  +--X  |  |  X--+  +--X--+  |  X--+  +--X--+
// |   \ |  | / \ |  | / \ |  | /|\ |  | /|\ |  | / \ |  | / \ |  | /|\ |  | /|\ |  | / \ |  | / \ |  | /|\ |  | /|\ |  | / \ |  | / \ |  | /|\ |  | /|\ |
// |    \|  |/   \|  |/   \|  |/ | \|  |/ | \|  |/   \|  |/   \|  |/ | \|  |/ | \|  |/   \|  |/   \|  |/ | \|  |/ | \|  |/   \|  |/   \|  |/ | \|  |/ | \|
// +-----+  +-----+  +-----+  +--+--+  +--+--+  +-----+  +-----+  +--+--+  +--+--+  +-----+  +-----+  +--+--+  +--+--+  +-----+  +-----+  +--+--+  +--+--+
//    0        1        2        3        4        5        6        7        8        9       10       11       12       13       14       15       16
//             0        1        2        3        4        5        6        7        8        9       10       11       12       13       14       15

// link index is a 4-bit addressing nesw 0-14 (1111 is the next (even) lod up, so in practices it's the same as a 0 link at level+1)

// === NOTE ===

// DICE describes a method where instead of up-linking to LoD level + 1, downlink to LoD level - 1. Also because the terrain can be arranged by quad-tree
// rather than fixed nodes everywhere, only the following 8 links are needed:

// +--+--+  +-----+  +--+--+  +--+--+  +--+--+  +-----+  +--+--+  +--+--+  +-----+  
// |\ | /|  |\   /|  |\ | /|  |\ | /|  |\ | /|  |\   /|  |\ | /|  |\ | /|  |\   /|  
// | \|/ |  | \ / |  | \|/ |  | \|/ |  | \|/ |  | \ / |  | \|/ |  | \|/ |  | \ / |  
// +--X--+  +--X--+  +--X  |  +--X--+  |  X--+  +--X  |  +--X  |  |  X--+  |  X--+  
// | /|\ |  | /|\ |  | /|\ |  | / \ |  | /|\ |  | /|\ |  | / \ |  | / \ |  | /|\ |  
// |/ | \|  |/ | \|  |/ | \|  |/   \|  |/ | \|  |/ | \|  |/   \|  |/   \|  |/ | \|  
// +--+--+  +--+--+  +--+--+  +-----+  +--+--+  +--+--+  +-----+  +-----+  +--+--+  
//   LoD       0        1        2        3        4        5        6        7
// btt index   7       11       13       14        3        9       12        6

#pragma once
#include <cstdint>

namespace ouro { namespace prim {

struct btt_vertex_t
{
	float x, y, z;
};

struct btt_node_t
{
  uint16_t tri[3]; // apex is always first
  uint16_t exterior; // 1 if tri has an edge on the outer vertex row/column of btt_vertices
};

// returns the vertex count for a patch. Use this to allocate enough room to 
// store these many vertices.
uint32_t btt_num_vertices(uint32_t max_depth);

// out_verts will receive the result of btt_num_vertices(max_depth) incremented
// by vert_stride.
void btt_vertices(uint32_t max_depth, btt_vertex_t* out_verts, uint32_t vert_stride);

// convert a linear index to the level and link it represents
void btt_calc_depth_and_link(uint32_t patch_index, uint16_t* out_level, uint16_t* out_link);

// convert a level and link to the linear index that contains its values
uint32_t btt_calc_index(uint32_t level, uint32_t link);

// returns the number of triangle at the specified depth
uint32_t btt_num_tris(uint32_t depth);

// returns the bytes requires to initialize a patch's 4 btts for further evaluation.
// Allocate this amount of memory and pass it to btt_patch_init and then to
// btt_patch_uplink_calc_num_indices.
uint32_t btt_calc_memory_bytes(uint32_t max_depth);

// populates the specified memory with a btt hierarchy ready for usage
void btt_init(void* btt_memory, uint16_t max_depth);

// returns the number of indices written to out_indices for the nth patch linear index
uint32_t btt_write_indices(void* btt_memory, uint16_t max_depth, uint32_t patch_index, uint16_t* out_indices);

}}
