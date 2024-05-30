/*----------------------------------------------------------------------------
 *               Video Buffer Pool Interface
 *
 *  Copyright  2004 Metrowerks Corp.
 *
 *  This provides a pool of pre-allocated buffers primarily for use by
 *  video drivers. Video buffers have special requirements on the
 *  memory they use. They need to be physically contiguous and, accessible
 *  with DMA. They are also generally large. Often embedded systems are memory
 *  constrained. Allocation of large contiguous buffers at boot time may
 *  not be a problem, but after memory has fragmented over time,
 *  allocation of the same buffers may be impossible.
 *
 *  This code provides a mechanism to pre-allocate key buffers before
 *  memory is fragmented and hold on to them until they are needed.
 *  
 *  Note: This interface must NOT be called at interrupt time.
 *        Any of the calls may sleep. This is because of the calls made 
 *        by the pool management code.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *--------------------------------------------------------------------------*/
#ifndef __ASM_VBUFPOOL_H
#define __ASM_VBUFPOOL_H

#include <linux/dma-mapping.h> // for dma_alloc_coherent & dma_free_coherent
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>

#define VBUF_TYPE_NON_CACHED   0
#define VBUF_TYPE_BUFFERABLE   1
#define VBUF_TYPE_CACHED       2

#define NUMBER_OF_VBUF_TYPES   3
#define VBUF_TYPE_DEFAULT      VBUF_TYPE_NON_CACHED

extern void *vbuf_alloc(size_t size, unsigned int type, dma_addr_t *dma_addr);
extern void  vbuf_free(void *vaddr, size_t size, unsigned int type, dma_addr_t dma_addr);
extern void vbuf_clean(void *vaddr, size_t size);
extern void vbuf_invalidate(void *vaddr, size_t size);
extern int   vbuf_pool_add(size_t size, unsigned int type, int count);
extern int   vbuf_pool_remove(size_t size, unsigned int type, int count, int force_flag);

/*--------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * The way video buffer memory is allocated from the system may need to
 * change depending on the architecture. These changes are isolated by the
 * following. These defines are used by the buffer management code and
 * should not be used by others.
 */
#define DECLARE_type_to_cache_flags  \
static unsigned long type_to_cache_flags[NUMBER_OF_VBUF_TYPES] = { \
	0, \
	PTE_BUFFERABLE, \
	(PTE_BUFFERABLE | PTE_CACHEABLE) \
}

#define DECLARE_type_to_name  \
static char *type_to_name[NUMBER_OF_VBUF_TYPES] = { \
	"non-cached", \
	"bufferable", \
	"cached" \
}

struct vbuf_name_value
{
	char *name;
	int   value;
};

#define DECLARE_vbuf_type_to_name  \
struct vbuf_name_value vbuf_type_array[] = { \
	{"nc", VBUF_TYPE_NON_CACHED}, \
	{"b",  VBUF_TYPE_BUFFERABLE}, \
	{"c",  VBUF_TYPE_CACHED}, \
	{0,0} \
}

void *
dma_alloc_coherent_nocheck(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp);

#define vbuf_pool_system_alloc(gfp, size, dma_addr, cache_flags) dma_alloc_coherent(NULL, size, dma_addr, gfp); // FIXME cache flags are lost
#define vbuf_pool_system_free(vaddr, size, dma_addr)             dma_free_coherent(NULL, size, vaddr, dma_addr);

#define vbuf_pool_system_clean_cache(vaddr, size) consistent_sync(vaddr, size, DMA_TO_DEVICE)
#define vbuf_pool_system_inv_cache(vaddr, size)   consistent_sync(vaddr, size, DMA_FROM_DEVICE)



#endif  /* __ASM_VBUFPOOL_H */
