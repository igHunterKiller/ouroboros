// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This header has accessor tools for working with a compact binary tree 
// that is stored as an array where btree[0] is a null/unused entry, 
// btree[1] is the root, btree[2] and btree[3] are the root's childen and 
// so on. To avoid confusion, the term "level" is used rather than depth. 
// There are important properties that occur at levels where the level of 
// the root is 0 rather than its depth which is 1.
//        0        // level-1 unused
//        1        // level0 root
//    2       3    // level1 first 2 children
//  4   5   6   7  // level2 next level after that
// 8 9 A B C D E F // level3 ...
//
// Useful properties:
// o the first node in each level is 2^level
// o the number of nodes in a level is 2^level
// o parents and children are easily accessible by bit-shifts/divides.
// o 0 can be used as a nullptr value
//
// Note: No bound checking is done in these functions, i.e. for the 
// above example, asking for the child of C or D will return a value
// though that may be outside of the range prepared by client code.
#pragma once
#include <oCore/bit.h>

// returns total number of nodes and number of levels (depth) of the 
// compact binary tree needed to represent the space. This rounds the
// number of leaf nodes up to the next power of two.
inline void cbtree_metrics(uint32_t num_leaf_nodes, uint32_t* out_num_nodes, uint32_t* out_num_levels)
{
  uint32_t pot = nextpow2(num_leaf_nodes);
  *out_num_nodes = pot * 2; // +1 for the null node
  *out_num_levels = log2i(pot) + 1;
}

// returns the node based on its index into the leaf level and the number of total nodes
// as returned by cbtree_metrics.
inline uint32_t cbtree_node_from_leaf_offset(uint32_t leaf_offset, uint32_t num_nodes)
{
	uint32_t first_leaf_node_index = num_nodes >> 1;
	return first_leaf_node_index + leaf_offset;
}

// returns the index of the left-most node in level (start of the list 
// of siblings). This is also the number of nodes in the level.
inline uint32_t cbtree_level_first_and_count(uint32_t level)
{
  return 1 << level;
}

// returns the level of the node
inline uint32_t cbtree_level(uint32_t node)
{
  return log2i(node);
}

// returns the index (starting from 0) into this level's 
// sibling list for the first node at this level
inline uint32_t cbtree_level_first_uncle(uint32_t node)
{
  return prevpow2(node);
}

// returns the node's parent or 0 node is the root
inline uint32_t cbtree_parent(uint32_t node)
{
  return node >> 1;
}

// return the left child of the node
inline uint32_t cbtree_left_child(uint32_t node)
{
  return node << 1;
}

// returns the right child of node
inline uint32_t cbtree_right_child(uint32_t node)
{
  return (node << 1) + 1;
}

// returns the other node that has the same parent
inline uint32_t cbtree_sibling(uint32_t node)
{
  int n = node & 1;
  return (node & ~1) | (n ^ n);
}
