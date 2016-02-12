// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

//    /\          /\     For each triangle, use edge-midpoint subdivision. 
//   /  \  ==>   /__\    Each subdivision adds at most 3 vertices and 9 
//  /    \      /\  /\   indices per triangle so allocate memory accordingly.
// /______\    /__\/__\ 

#include <cstdint>

namespace ouro {

// Use these to size buffers properly
uint16_t calc_max_indices(uint16_t base_num_indices, uint16_t divide);
uint16_t calc_max_vertices(uint16_t base_num_vertices, uint16_t divide);

// First call this function with the desired divide, null for all pointers and values and initial counts of the
// base mesh to subdivide. The counters will be updated with the maximum counts that could be applied for the
// given subdivide. Allocate the memory for the buffer to accomodate the maximums, then call this function again
// with proper parameters - remember to reset num_indices and num_vertices to inital counts. When the function
// is finished, num_indices and num_vertices will hold the actual count of validly subdivided mesh data.
void subdivide(uint16_t divide   // number of times to actually do the subdivision (0 is noop)
	, uint16_t* indices            // index array containing initial faces, but sized to receive all subdivision
	, uint32_t& inout_num_indices  // user should initialize to base num indices, this is grown as subdivision occurs
	, float* vertices              // contains initial vertices (xyz triplets), but sized to receive all subdivision
	, uint16_t& inout_num_vertices // user should initialize to base num vertices, this is grown as subdivision occurs
	, void (*new_vertex)(uint16_t new_index, uint16_t a, uint16_t b, void* user) // user callback called when a new vertex is added half-way between vertices a & b at new_index
	, void* user);                 // user contetx for new_vertex
}
