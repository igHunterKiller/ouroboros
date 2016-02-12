// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/btt.h>
#include <oCore/cbtree.h>
#include <memory.h>

namespace ouro { namespace prim {

static uint32_t btt_num_row_verts(uint32_t max_depth)
{
	return (1 << (((max_depth + 1) & ~1) / 2)) + 1;
}

uint32_t btt_num_vertices(uint32_t max_depth)
{
	uint32_t n = btt_num_row_verts(max_depth);
	return n * n;
}

void btt_vertices(uint32_t max_depth, btt_vertex_t* out_verts, uint32_t vert_stride)
{
	uint32_t num_row_verts = btt_num_row_verts(max_depth);

	btt_vertex_t pos = { 0.0f, 0.0f, 0.0f };
	float step = 1.0f / (float)(num_row_verts - 1);
  for (uint32_t y = 0; y < num_row_verts; y++)
  {
    pos.x = 0.0f;
    for (uint32_t x = 0; x < num_row_verts; x++)
    {
      *out_verts = pos;
      out_verts = (btt_vertex_t*)((char*)out_verts + vert_stride);
      pos.x += step;
    }
    pos.y += step;
  }
}

void btt_calc_depth_and_link(uint32_t patch_index, uint16_t* out_level, uint16_t* out_link)
{
	if (patch_index == 0)
	{
		*out_link  = 0;
		*out_level = 0;
	}

	else
	{
		uint16_t patch_index_minus1 = (uint16_t)patch_index - 1;

		*out_level = (patch_index_minus1 / 15) + 1;
		*out_link  =  patch_index_minus1 % 15;
	}
}

uint32_t btt_calc_index(uint32_t level, uint32_t link)
{
	return (level - 1) * 15 + 1;
}

uint32_t btt_num_tris(uint32_t depth)
{
	return 1 << (depth + 1); // @tony: currently accomodates the worst-case uplink, which is really depth+1...
}

// returns true if out_midpoint contains the index of a point in the middle of 
// idx0 and idx1 on a square grid that is num_row_verts * num_row_verts.
static bool btt_midpoint(uint16_t idx0, uint16_t idx1, uint16_t num_row_verts, uint16_t* out_midpoint)
{
  // convert to 2d coords
  uint16_t idx0_x = idx0 % num_row_verts;
  uint16_t idx0_y = idx0 / num_row_verts;

  uint16_t idx1_x = idx1 % num_row_verts;
  uint16_t idx1_y = idx1 / num_row_verts;
  
  // find the midpoint, give up if it won't divide evenly by two
	uint16_t sum_x = idx0_x + idx1_x;
	if (sum_x & 1)
		return false;

	uint16_t sum_y = idx0_y + idx1_y;
	if (sum_y & 1)
		return false;

  uint16_t mid_x = sum_x >> 1;
  uint16_t mid_y = sum_y >> 1;
    
  // convert back to linear index
  *out_midpoint = mid_y * num_row_verts + mid_x;
  return true;
}

static bool btt_is_exterior(uint16_t x, uint16_t y, uint16_t num_row_verts_minus_1)
{
	return x == 0 || x == num_row_verts_minus_1 || y == 0 || y == num_row_verts_minus_1;
}

static bool btt_is_exterior(const uint16_t tri[3], uint16_t num_row_verts)
{
	uint16_t x0 = tri[0] % num_row_verts;
	uint16_t y0 = tri[0] / num_row_verts;
	uint16_t x1 = tri[1] % num_row_verts;
	uint16_t y1 = tri[1] / num_row_verts;
	uint16_t x2 = tri[2] % num_row_verts;
	uint16_t y2 = tri[2] / num_row_verts;
	uint16_t last_row_col = num_row_verts - 1;
	uint32_t ext0 = btt_is_exterior(x0, y0, last_row_col) & 1;
	uint32_t ext1 = btt_is_exterior(x1, y1, last_row_col) & 1;
	uint32_t ext2 = btt_is_exterior(x2, y2, last_row_col) & 1;
	return (ext0 + ext1 + ext2) > 1;
}

static bool btt_subdivide(btt_node_t* btt, uint32_t node, uint16_t num_row_verts)
{
	btt_node_t& n = btt[node];
	uint16_t midpoint;
  if (!btt_midpoint(n.tri[1], n.tri[2], num_row_verts, &midpoint))
    return false;

  btt_node_t& l = btt[cbtree_left_child(node)];
  l.tri[0]   = midpoint;
  l.tri[1]   = n.tri[0];
  l.tri[2]   = n.tri[1];
  l.exterior = btt_is_exterior(l.tri, num_row_verts) & 1;
  
  btt_node_t& r = btt[cbtree_right_child(node)];
  r.tri[0]   = midpoint;
  r.tri[1]   = n.tri[2];
  r.tri[2]   = n.tri[0];
  r.exterior = btt_is_exterior(r.tri, num_row_verts) & 1;
 
	return true;
}

static void btt_subdivide_recursively(btt_node_t* btt, uint32_t node, uint16_t num_row_verts)
{
  if (btt_subdivide(btt, node, num_row_verts))
  {
    btt_subdivide_recursively(btt, cbtree_left_child(node), num_row_verts);
    btt_subdivide_recursively(btt, cbtree_right_child(node), num_row_verts);
  }
}

// returns the bytes required for one btt
static uint32_t btt_calc_memory_bytes_internal(uint32_t max_depth)
{
	uint32_t num_nodes, num_levels;
	cbtree_metrics(btt_num_tris(max_depth), &num_nodes, &num_levels);
	uint32_t btt_bytes = num_nodes * sizeof(btt_node_t);
	return btt_bytes;
}

// initializes btt to contain btt_node_t's for every level of the binary triangle tree
// based on the max depth desired, use btt_num_row_verts for num_row_verts to get the
// indexing correct. Use cbtree.h API to access btt as a binary tree. The indexing 
// here requiriing knowledge of the vertex field means that btt is somewhat coupled to
// 4-btt-on-a-patch usage.
static void btt_init(btt_node_t* btt, const uint16_t tri[3], uint16_t num_row_verts)
{
	memset(btt, 0, sizeof(btt));
  btt_node_t& root = btt[1];
  root.tri[0]   = tri[0];
  root.tri[1]   = tri[1];
  root.tri[2]   = tri[2];
  root.exterior = 1;
  btt_subdivide_recursively(btt, 1, num_row_verts);
}

uint32_t btt_calc_memory_bytes(uint32_t max_depth)
{
	uint32_t btt_bytes = btt_calc_memory_bytes_internal(max_depth);
	return btt_bytes * 4;
}

// writes all indices for the triangles at depth in the btt and returns the pointer
// for future writes. If subdivide_exteriors is specified then exterior triangles are split 
// one more time. NOTE: ensure add_link is not specified for the max_depth, since it will 
// index into unallocated memory.
static uint16_t* btt_indices(const btt_node_t* btt, uint16_t depth, bool subdivide_exteriors, uint16_t* out_indices)
{
	uint32_t node = cbtree_level_first_and_count(depth - 1);
	uint32_t end  = node + node;

	while (node < end)
	{
		auto& n = btt[node];

		if (n.exterior && subdivide_exteriors)
		{
			auto& l = btt[cbtree_left_child(node)];
			*out_indices++ = l.tri[0];
			*out_indices++ = l.tri[1];
			*out_indices++ = l.tri[2];

			auto& r = btt[cbtree_right_child(node)];
			*out_indices++ = r.tri[0];
			*out_indices++ = r.tri[1];
			*out_indices++ = r.tri[2];
		}

		else
		{
			*out_indices++ = n.tri[0];
			*out_indices++ = n.tri[1];
			*out_indices++ = n.tri[2];
		}

		node++;
	}

	return out_indices;
}

void btt_init(void* btt_memory, uint16_t max_depth)
{
	uint16_t num_row_verts = (uint16_t)btt_num_row_verts(max_depth);
	uint32_t num_nodes, num_levels;
	cbtree_metrics(btt_num_tris(max_depth), &num_nodes, &num_levels);

	auto n                 = (btt_node_t*)btt_memory;
	auto e                 = n + num_nodes;
	auto s                 = e + num_nodes;
	auto w                 = s + num_nodes;

	uint16_t num_verts     = num_row_verts * num_row_verts;
	uint16_t ne            = num_verts - 1;
	uint16_t se            = num_row_verts - 1;
	uint16_t sw            = 0;
	uint16_t nw            = num_verts - num_row_verts;
	uint16_t mi            = num_verts / 2;

	// level 0 is handled as a special case the 
	// btt's start at level 1, populated here
	const uint16_t tris[4][3] = // nw-+-ne
	{														// |\   /|
		{ mi, nw, ne }, 					// | \ / |
		{ mi, ne, se },						// |  mi |
		{ mi, se, sw },						// | / \ |
		{ mi, sw, nw },						// |/   \|
	};													// sw---se

	btt_init(n, tris[0], num_row_verts);
	btt_init(e, tris[1], num_row_verts);
	btt_init(s, tris[2], num_row_verts);
	btt_init(w, tris[3], num_row_verts);
}

uint32_t btt_write_indices(void* btt_memory, uint16_t max_depth, uint32_t patch_index, uint16_t* out_indices)
{
	// Even Odd OddLinks[14] Even Odd OddLinks[14] ...

	uint16_t* indices      = out_indices;

	uint16_t num_row_verts = (uint16_t)btt_num_row_verts(max_depth);
	uint32_t num_nodes, num_levels;
	cbtree_metrics(btt_num_tris(max_depth), &num_nodes, &num_levels);

	auto n                 = (btt_node_t*)btt_memory;
	auto e                 = n + num_nodes;
	auto s                 = e + num_nodes;
	auto w                 = s + num_nodes;

	if (patch_index == 0)
	{
		uint16_t num_verts   = num_row_verts * num_row_verts;
		uint16_t ne          = num_verts - 1;
		uint16_t se          = num_row_verts - 1;
		uint16_t sw          = 0;
		uint16_t nw          = num_verts - num_row_verts;

		*indices++ = nw; *indices++ = se; *indices++ = sw;
		*indices++ = ne; *indices++ = se; *indices++ = nw;
	}

	else
	{
		uint16_t level, nesw_index;
		btt_calc_depth_and_link(patch_index, &level, &nesw_index);

		indices = btt_indices(n, level, !!(nesw_index & 8), indices);
		indices = btt_indices(e, level, !!(nesw_index & 4), indices);
		indices = btt_indices(s, level, !!(nesw_index & 2), indices);
		indices = btt_indices(w, level, !!(nesw_index & 1), indices);
	}

	return (uint32_t)(indices - out_indices);
}

}}
