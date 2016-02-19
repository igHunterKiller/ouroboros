// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/assert.h>
#include <oMemory/sbb.h>
#include <oCore/cbtree.h>
#include <stdexcept>

#define SBB_FATAL(msg) oThrow(std::errc::invalid_argument, msg)
#define SBB_MAX(x,y) ((x) > (y) ? (x) : (y))
#define SBB_ALIGNED(x,align) (((((uintptr_t)x) + (align) - 1) & ~(align-1)) == (uintptr_t)x)

typedef size_t sbb_page_t;
typedef uint32_t sbb_node_t;

enum sbb_constants
{
#ifdef _M_X64
	kPageBitsLog2 = 6,
#else
	kPageBitsLog2 = 5,
#endif
	kPageBytes = sizeof(sbb_page_t),
	kPageBits = kPageBytes * 8,
	kPageBitmask = kPageBits - 1,
	kPageMaxBits = kPageBits - 1,
	kRoot = 1,
};

static const sbb_page_t kRootMask = sbb_page_t(1) << (kPageBits - 2);

struct sbb_bookkeeping_t
{
	void* arena;
	size_t arena_bytes; // must be pow2
	uint32_t min_block_size; // must be pow2
	uint32_t min_block_size_log2; // log2i(min_block_size)
	uint32_t num_nodes;
	uint32_t unused; // keeps size consistently aligned between 64- and 32-bit compiles
};

// utils to convert a node index into its page and bit/bitmask within that page or a user pointer
static inline sbb_page_t* sbb_page(sbb_bookkeeping_t* bookkeeping, sbb_node_t node) { return ((sbb_page_t*)(bookkeeping + 1)) + (node >> kPageBitsLog2); }
static inline int sbb_pagebit(sbb_node_t node) { return kPageMaxBits - (node & kPageBitmask); }
static inline sbb_page_t sbb_pagemask(int bit_index) { return sbb_page_t(1) << bit_index; }
static inline void* to_ptr(sbb_bookkeeping_t* bookkeeping, sbb_node_t node, size_t level_block_size) { return (node - cbtree_level_first_uncle(node)) * level_block_size + (uint8_t*)bookkeeping->arena; }

// utils to test and mark node values
static inline bool marked_free(sbb_page_t* page, sbb_page_t mask) { return (*page & mask) == mask; }
static inline void mark_used(sbb_page_t* page, sbb_page_t mask) { *page &=~ mask; }
static inline void mark_free(sbb_page_t* page, sbb_page_t mask) { *page |= mask; }

#define SBB_CHECK_ALIGNMENT \
	if (!ispow2(bytes)) SBB_FATAL("arena must be a power of two"); \
	if (!ispow2(min_block_size)) SBB_FATAL("min_block_size must be a power of two"); \
	if ((size_t)(uint32_t)min_block_size != min_block_size) SBB_FATAL("min_block_size must fit in uint32_t");

size_t sbb_bookkeeping_size(size_t bytes, size_t min_block_size)
{
	SBB_CHECK_ALIGNMENT
	sbb_node_t nnodes, nlevels;
	cbtree_metrics(sbb_node_t(bytes / min_block_size), &nnodes, &nlevels);
	sbb_node_t npages = SBB_MAX(1, nnodes >> kPageBitsLog2);
	return npages * sizeof(sbb_page_t) + sizeof(sbb_bookkeeping_t);
}

sbb_t sbb_create(void* arena, size_t bytes, size_t min_block_size, void* bookkeeping)
{
	sbb_bookkeeping_t* sbb = (sbb_bookkeeping_t*)bookkeeping;
	if (!bookkeeping)
		SBB_FATAL("a valid bookkeeping memory must be specified");
	SBB_CHECK_ALIGNMENT
	if (arena && !SBB_ALIGNED(arena, min_block_size))
		SBB_FATAL("arena must be valid and aligned to min_block_size");

	const uint32_t min_block_size_log2 = log2i(min_block_size);

	sbb->arena = arena;
	sbb->arena_bytes = bytes;
	sbb->min_block_size = (uint32_t)min_block_size;
	sbb->min_block_size_log2 = min_block_size_log2;
	uint32_t nlevels;
	cbtree_metrics(sbb_node_t(bytes >> sbb->min_block_size_log2), &sbb->num_nodes, &nlevels);
	sbb->unused = 0;

	// the top bit of the first page is a null since the binary tree's 
	// root starts a 1, so mark everything else as available
	sbb_page_t* page = sbb_page(sbb, 0);
	const sbb_page_t* page_end = page + SBB_MAX(1, sbb->num_nodes >> kPageBitsLog2);
	*page++ = ~(sbb_pagemask(kPageBits-1));
	for (; page < page_end; page++)
		*page = sbb_page_t(-1);

	return (sbb_t)sbb;
}

void sbb_destroy(sbb_t sbb)
{
}

static void* sbb_memalign_internal(sbb_bookkeeping_t* bookkeeping, sbb_page_t* node_page, sbb_page_t node_mask, sbb_node_t node, size_t block_size, size_t level_block_size)
{
	if (block_size < level_block_size) // recurse to the best-fit level
	{
		auto left = cbtree_left_child(node);
		auto children_page = sbb_page(bookkeeping, left);
		auto left_mask = sbb_pagemask(sbb_pagebit(left));
		auto right_mask = left_mask >> 1;

		if (!marked_free(node_page, node_mask) && marked_free(children_page, left_mask | right_mask))
			return nullptr;

		size_t child_level_block_size = level_block_size >> 1;
		void* ptr = sbb_memalign_internal(bookkeeping, children_page, left_mask, left, block_size, child_level_block_size);
		if (!ptr)
			ptr = sbb_memalign_internal(bookkeeping, children_page, right_mask, left + 1, block_size, child_level_block_size);

		if (ptr && marked_free(node_page, node_mask)) // dirty bits of parents
			mark_used(node_page, node_mask);

		return ptr;
	}

	if (marked_free(node_page, node_mask))
	{
		mark_used(node_page, node_mask);
		return to_ptr(bookkeeping, node, block_size);
	}

	return nullptr;
}

void* sbb_memalign(sbb_t sbb, size_t align, size_t size)
{
	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	size_t block_size = SBB_MAX(bookkeeping->min_block_size, nextpow2(size));
	size_t level_block_size = bookkeeping->arena_bytes;
	if (block_size > level_block_size)
		return nullptr;
	return sbb_memalign_internal(bookkeeping, (sbb_page_t*)(bookkeeping + 1), kRootMask, kRoot, block_size, level_block_size);
}

void sbb_free(sbb_t sbb, void* ptr)
{
	if (!ptr)
		return;

	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	void* arena = bookkeeping->arena;
	const size_t arena_size = bookkeeping->arena_bytes;

	if (ptr < arena || ptr >= ((uint8_t*)arena + arena_size))
		SBB_FATAL("ptr is not from this sbb allocator");

	// find ptr as a leaf node since ptr shares alignment with all left children and the size isn't known
	const size_t byte_offset = size_t((uint8_t*)ptr - (uint8_t*)arena);
	const uint32_t min_block_size_log2 = bookkeeping->min_block_size_log2;
	const uint32_t offset = uint32_t(byte_offset >> min_block_size_log2);
	auto node = cbtree_node_from_leaf_offset(offset, bookkeeping->num_nodes);

	// walk up from there to the first allocation
	sbb_page_t* node_page = sbb_page(bookkeeping, node);
	int node_bit = sbb_pagebit(node);
	sbb_page_t node_mask = sbb_pagemask(node_bit);
	while (marked_free(node_page, node_mask))
	{
		node = cbtree_parent(node);
		node_page = sbb_page(bookkeeping, node);
		node_bit = sbb_pagebit(node);
		node_mask = sbb_pagemask(node_bit);
	}

	// mark this free and propagate the flag up the hierarchy as long as sibling/buddy is free
	mark_free(node_page, node_mask);
	while (marked_free(node_page, sbb_pagemask(node_bit ^ 1)))
	{
		node = cbtree_parent(node);
		node_page = sbb_page(bookkeeping, node);
		node_bit = sbb_pagebit(node);
		node_mask = sbb_pagemask(node_bit);
		mark_free(node_page, node_mask);
	}
}

// convert a node index to a page + bit index
void* sbb_malloc(sbb_t sbb, size_t size)
{
	return sbb_memalign(sbb, sbb_min_block_size(sbb), size);
}

void* sbb_realloc(sbb_t sbb, void* ptr, size_t bytes)
{
	if (!ptr)
		return nullptr;

	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	void* arena = bookkeeping->arena;
	const size_t arena_size = bookkeeping->arena_bytes;

	if (ptr < arena || ptr >= ((uint8_t*)arena + arena_size))
		SBB_FATAL("ptr is not from this sbb allocator");

	size_t level_block_size = bookkeeping->min_block_size;
	size_t block_size = std::max(level_block_size, nextpow2(bytes));
	if (block_size == level_block_size)
		return ptr;

	// find ptr as a leaf node since ptr shares alignment with all left children and the size isn't known
	const size_t byte_offset = size_t((uint8_t*)ptr - (uint8_t*)arena);
	const uint32_t min_block_size_log2 = bookkeeping->min_block_size_log2;
	const uint32_t offset = uint32_t(byte_offset >> min_block_size_log2);
	auto node = cbtree_node_from_leaf_offset(offset, bookkeeping->num_nodes);

	// walk up from there to the first allocation
	sbb_page_t* node_page = sbb_page(bookkeeping, node);
	int node_bit = sbb_pagebit(node);
	sbb_page_t node_mask = sbb_pagemask(node_bit);
	while (marked_free(node_page, node_mask))
	{
		node = cbtree_parent(node);
		node_page = sbb_page(bookkeeping, node);
		node_bit = sbb_pagebit(node);
		node_mask = sbb_pagemask(node_bit);
		level_block_size <<= 1;
	}

	size_t alloc_block_size = level_block_size;

	// subdivide down to best-fit
	while (block_size < level_block_size)
	{
		node = cbtree_left_child(node);
		node_page = sbb_page(bookkeeping, node);
		node_bit = sbb_pagebit(node);
		node_mask = sbb_pagemask(node_bit);
		mark_used(node_page, node_mask);
		level_block_size >>= 1;
	}

	// check for adjacent free space
	bool adjacent_free = true;
	while (block_size > level_block_size)
	{
		if (!marked_free(node_page, sbb_pagemask(node_bit ^ 1)))
		{
			adjacent_free = false;
			break;
		}
		node = cbtree_parent(node);
		node_page = sbb_page(bookkeeping, node);
		node_bit = sbb_pagebit(node);
		level_block_size <<= 1;
	}

	// now that there's a known path to full memory, mark the path
	if (adjacent_free)
	{
		auto mark_node = node;
		sbb_page_t* mark_node_page, mark_node_mask;
		int mark_node_bit;
		while (level_block_size >= alloc_block_size)
		{
			mark_node = cbtree_left_child(mark_node);
			mark_node_page = sbb_page(bookkeeping, mark_node);
			mark_node_bit = sbb_pagebit(mark_node);
			mark_node_mask = sbb_pagemask(mark_node_bit);
			mark_free(mark_node_page, mark_node_bit);
			level_block_size >>= 1;
		}

		return to_ptr(bookkeeping, node, block_size);
	}

	// try a full malloc
	void* new_ptr = sbb_memalign(sbb, bookkeeping->min_block_size, block_size);
	if (new_ptr)
	{
		memcpy(new_ptr, ptr, alloc_block_size);
		sbb_free(sbb, ptr);
	}

	return new_ptr;
}

void* sbb_arena(sbb_t sbb)
{
	return ((sbb_bookkeeping_t*)sbb)->arena;
}

size_t sbb_arena_bytes(sbb_t sbb)
{
	return ((sbb_bookkeeping_t*)sbb)->arena_bytes;
}

void* sbb_bookkeeping(sbb_t sbb)
{
	return sbb;
}

size_t sbb_block_size(sbb_t sbb, void* ptr)
{
	if (!ptr)
		return 0;

	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	void* arena = bookkeeping->arena;
	const size_t arena_size = bookkeeping->arena_bytes;

	if (ptr < arena || ptr >= ((uint8_t*)arena + arena_size))
		SBB_FATAL("ptr is not from this sbb allocator");

	// find ptr as a leaf node since ptr shares alignment with all left children and the size isn't known
	const size_t byte_offset = size_t((uint8_t*)ptr - (uint8_t*)arena);
	const uint32_t min_block_size_log2 = bookkeeping->min_block_size_log2;
	const uint32_t offset = uint32_t(byte_offset >> min_block_size_log2);
	auto node = cbtree_node_from_leaf_offset(offset, bookkeeping->num_nodes);
	size_t block_size = bookkeeping->min_block_size;

	// walk up from there to the first allocation
	sbb_page_t* node_page = sbb_page(bookkeeping, node);
	int node_bit = sbb_pagebit(node);
	sbb_page_t node_mask = sbb_pagemask(node_bit);
	while (marked_free(node_page, node_mask))
	{
		node = cbtree_parent(node);
		node_page = sbb_page(bookkeeping, node);
		node_bit = sbb_pagebit(node);
		node_mask = sbb_pagemask(node_bit);
		block_size <<= 1;
	}

	return block_size > arena_size ? 0 : block_size;
}

size_t sbb_min_block_size(sbb_t sbb)
{
	return ((sbb_bookkeeping_t*)sbb)->min_block_size;
}

static inline size_t sbb_max_free_block_size_internal(sbb_bookkeeping_t* bookkeeping, sbb_page_t* node_page, sbb_page_t node_mask, sbb_node_t node, size_t level_block_size)
{
	// if this is marked used and both children are free it means this is an allocated node
	// so there's no more memory here.
	if (marked_free(node_page, node_mask))
		return level_block_size;

	auto left = cbtree_left_child(node);
	auto children_page = sbb_page(bookkeeping, left);
	auto left_mask = sbb_pagemask(sbb_pagebit(left));
	auto right_mask = left_mask >> 1;

	size_t child_level_block_size = level_block_size >> 1;
	size_t lsize = sbb_max_free_block_size_internal(bookkeeping, children_page, left_mask, left, child_level_block_size);
	size_t rsize = sbb_max_free_block_size_internal(bookkeeping, children_page, right_mask, left + 1, child_level_block_size);

	return SBB_MAX(lsize, rsize);
}

size_t sbb_max_free_block_size(sbb_t sbb)
{
	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	return sbb_max_free_block_size_internal(bookkeeping, (sbb_page_t*)(bookkeeping + 1), kRootMask, kRoot, bookkeeping->arena_bytes);
}

static inline size_t sbb_num_free_blocks_internal(sbb_bookkeeping_t* bookkeeping, sbb_page_t* node_page, sbb_page_t node_mask, sbb_node_t node)
{
	// this means all subnodes are free too making this undivided free block
	if (marked_free(node_page, node_mask))
		return 1;

	auto left = cbtree_left_child(node);

	// used leaf node
	if (left >= bookkeeping->num_nodes)
		return 0;

	auto children_page = sbb_page(bookkeeping, left);
	auto left_mask = sbb_pagemask(sbb_pagebit(left));

	return sbb_num_free_blocks_internal(bookkeeping, children_page, left_mask, left) 
		+ sbb_num_free_blocks_internal(bookkeeping, children_page, left_mask >> 1, left + 1);
}

size_t sbb_num_free_blocks(sbb_t sbb)
{
	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	return sbb_num_free_blocks_internal(bookkeeping, (sbb_page_t*)(bookkeeping + 1), kRootMask, kRoot);
}

size_t sbb_overhead(sbb_t sbb)
{
	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	return sbb_bookkeeping_size(bookkeeping->arena_bytes, bookkeeping->min_block_size);
}

void sbb_walk_heap_internal(sbb_bookkeeping_t* bookkeeping, sbb_page_t* node_page, sbb_page_t node_mask, sbb_node_t node, size_t level_block_size, sbb_walker walker, void* user)
{
	int used = 0;
	if (!marked_free(node_page, node_mask)) // recurse
	{
		auto children_level_block_size = level_block_size >> 1;
		auto left = cbtree_left_child(node);
		auto children_page = sbb_page(bookkeeping, left);
		auto left_mask = sbb_pagemask(sbb_pagebit(left));
		auto right_mask = left_mask >> 1;
		auto both_mask = left_mask | right_mask;

		if (left >= bookkeeping->num_nodes || (*children_page & both_mask) == both_mask)
			used = 1;
		else
		{
			sbb_walk_heap_internal(bookkeeping, children_page, left_mask, left, children_level_block_size, walker, user);
			sbb_walk_heap_internal(bookkeeping, children_page, right_mask, left + 1, children_level_block_size, walker, user);
			return;
		}
	}

	walker(to_ptr(bookkeeping, node, level_block_size), level_block_size, used, user);
}

void sbb_walk_heap(sbb_t sbb, sbb_walker walker, void* user)
{
	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	sbb_walk_heap_internal(bookkeeping, (sbb_page_t*)(bookkeeping + 1), kRootMask, kRoot, bookkeeping->arena_bytes, walker, user);
}

static bool subnodes_free(sbb_bookkeeping_t* bookkeeping, sbb_node_t node)
{
	sbb_node_t left = cbtree_left_child(node);
	if (left >= bookkeeping->num_nodes)
		return true;
	
	auto page = sbb_page(bookkeeping, left);
	int bit = sbb_pagebit(left);
	auto mask = sbb_pagemask(bit);
	if (!marked_free(page, mask)) // check left
		return false;

	mask = sbb_pagemask(bit+1);
	return marked_free(page, mask); // check right
}

static bool check_node(sbb_bookkeeping_t* bookkeeping, sbb_node_t node)
{
	auto page = sbb_page(bookkeeping, node);
	int bit = sbb_pagebit(node);
	auto mask = sbb_pagemask(bit);
	
	if (marked_free(page, mask))
	{
		if (!subnodes_free(bookkeeping, node))
			return false;
		return true;
	}

	auto left = cbtree_left_child(node);
	return check_node(bookkeeping, left) && check_node(bookkeeping, left+1);
}

bool sbb_check_heap(sbb_t sbb)
{
	sbb_bookkeeping_t* bookkeeping = (sbb_bookkeeping_t*)sbb;
	return check_node(bookkeeping, 1);
}
