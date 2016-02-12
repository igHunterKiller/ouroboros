/*
 * jmemnobs.c
 *
 * Copyright (C) 1992-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides a really simple implementation of the system-
 * dependent portion of the JPEG memory manager.  This implementation
 * assumes that no backing-store files are needed: all required space
 * can be obtained from malloc().
 * This is very portable in the sense that it'll compile on almost anything,
 * but you'd better have lots of main memory (or virtual memory) if you want
 * to process big images.
 * Note that the max_memory_to_use option is ignored by this implementation.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		/* import the system-dependent declarations */
#include <assert.h>

#ifndef HAVE_STDLIB_H		/* <stdlib.h> should declare malloc(),free() */
extern void * malloc JPP((size_t size));
extern void free JPP((void *ptr));
#endif


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	return cinfo->alloc->get_small(cinfo, sizeofobject); // ouroboros: we reroute through our extension allocator
	//return (void *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	cinfo->alloc->free_small(cinfo, object, sizeofobject); // ouroboros: we reroute through our extension allocator
	//free(object);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	return cinfo->alloc->get_large(cinfo, sizeofobject); // ouroboros: we reroute through our extension allocator
	//return (void FAR *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	cinfo->alloc->free_large(cinfo, object, sizeofobject); // ouroboros: we reroute through our extension allocator
	//free(object);
}


/*
 * This routine computes the total memory space available for allocation.
 * Here we always say, "we got all you want bud!"
 */

GLOBAL(size_t)
jpeg_mem_available (j_common_ptr cinfo, size_t min_bytes_needed,
		    size_t max_bytes_needed, size_t already_allocated)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	return cinfo->alloc->mem_available(cinfo, min_bytes_needed, max_bytes_needed, already_allocated); // ouroboros: we reroute through our extension allocator
	//return max_bytes_needed;
}


/*
 * Backing store (temporary file) management.
 * Since jpeg_mem_available always promised the moon,
 * this should never be called and we can just error out.
 */

GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
  ERREXIT(cinfo, JERR_NO_BACKING_STORE);
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  Here, there isn't any.
 */

GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	return cinfo->alloc->init(cinfo); // ouroboros: we reroute through our extension allocator
	//return 0;			/* just set max_memory_to_use to 0 */
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
	assert(cinfo->alloc); // ouroboros: must have an allocator
	cinfo->alloc->term(cinfo); // ouroboros: we reroute through our extension allocator
	/* no work */
}

/* ouroboros: here are the default allocators, if we aren't going to use a custom allocator. basically they do what libjpeg did , i.e. just pass through
	code taken from original libjpeg jmemnobs.c
*/
METHODDEF(void *)
jpeg_get_small_default (j_common_ptr cinfo, size_t sizeofobject)
{
	return (void *) malloc(sizeofobject);
}

METHODDEF(void)
jpeg_free_small_default (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
	free(object);
}

METHODDEF(void FAR *)
jpeg_get_large_default (j_common_ptr cinfo, size_t sizeofobject)
{
	return (void FAR *) malloc(sizeofobject);
}

METHODDEF(void)
jpeg_free_large_default (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
	free(object);
}

METHODDEF(size_t)
jpeg_mem_available_default (j_common_ptr cinfo, size_t min_bytes_needed,
		    size_t max_bytes_needed, size_t already_allocated)
{
	return max_bytes_needed;
}

METHODDEF(long)
jpeg_mem_init_default (j_common_ptr cinfo)
{
	return 0;			/* just set max_memory_to_use to 0 */
}

METHODDEF(void)
jpeg_mem_term_default (j_common_ptr cinfo)
{
	/* no work */
}

/*
 * Fill in the standard alloc methods in a ouro_jpeg_memory_alloc object.
 * Typical call is:
 *	struct jpeg_compress_struct cinfo;
 *	struct ouro_jpeg_memory_alloc alloc;
 *
 *	cinfo.alloc = ouro_jpeg_std_alloc(&alloc);
 * If providing your own alloc, you don't need to call this.
 */

GLOBAL(struct ouro_jpeg_memory_alloc *)
ouro_jpeg_std_alloc (struct ouro_jpeg_memory_alloc * alloc)
{
	alloc->get_small = jpeg_get_small_default;
	alloc->free_small = jpeg_free_small_default;
	alloc->get_large = jpeg_get_large_default;
	alloc->free_large = jpeg_free_large_default;
	alloc->mem_available = jpeg_mem_available_default;
	alloc->init = jpeg_mem_init_default;
	alloc->term = jpeg_mem_term_default;

	return alloc;
}
