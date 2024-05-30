/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 07/08/2007
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_MEMORY_H__
#define __ASM_ARCH_MXC_MEMORY_H__

#include <linux/version.h>
#include <asm/arch/hardware.h>
#ifndef PAGE_SHIFT
#include <asm/page.h>
#endif

/*!
 * @defgroup Memory_MX27 Memory Map
 * @ingroup MSL_MX27
 */
/*!
 * @defgroup Memory_MX31 Memory Map
 * @ingroup MSL_MX31
 */

/*!
 * @file arch-mxc/memory.h
 * @brief This file contains macros needed by the Linux kernel and drivers.
 *
 * @ingroup Memory_MX27 Memory_MX31
 */
#ifndef __ASSEMBLY__

#define DMA_ZONE_SIZE   ((SZ_8M + CONSISTENT_DMA_SIZE) >> PAGE_SHIFT)

static inline void __arch_adjust_zones(int node, unsigned long *zone_size,
				       unsigned long *zhole_size)
{
	if (node != 0) return;
	/* Only the first part of memory is usable for DMA */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18))
	zone_size[1] = zone_size[0] - DMA_ZONE_SIZE;
	zhole_size[1] = zhole_size[0];
#else
	zone_size[2] = zone_size[0] - DMA_ZONE_SIZE;
	zhole_size[2] = zhole_size[0];
#endif
	zone_size[0] = DMA_ZONE_SIZE;
	zhole_size[0] = 0;
}

#define arch_adjust_zones(node, size, holes) \
			  __arch_adjust_zones(node, size, holes)

#define ISA_DMA_THRESHOLD   (PHYS_OFFSET + (DMA_ZONE_SIZE << PAGE_SHIFT) - 1)

#endif

/*!
 * Virtual view <-> DMA view memory address translations
 * This macro is used to translate the virtual address to an address
 * suitable to be passed to set_dma_addr()
 */
#define __virt_to_bus(a)	__virt_to_phys(a)

/*!
 * Used to convert an address for DMA operations to an address that the
 * kernel can use.
 */
#define __bus_to_virt(a)	__phys_to_virt(a)

#ifdef CONFIG_DISCONTIGMEM

/*!
 * The MXC's memory is physically contiguous, but to
 * support and test Memory Type Based Allocation, we need
 * to artificially create some memory banks. Our goal
 * it to have a minimum of 4 nodes of the same size
 * and to try to have a Node size be 16MiB when  possible.
 */
#if (SDRAM_MEM_SIZE == SZ_256M)
#define        NODES_SHIFT	2	/* 4 Nodes */
#define NODE_MAX_MEM_SHIFT	26	/* 64 MiB */
#elif (SDRAM_MEM_SIZE == SZ_128M)
#define        NODES_SHIFT	2	/* 4 Nodes */
#define NODE_MAX_MEM_SHIFT	25	/* 32 MiB */
#elif (SDRAM_MEM_SIZE == SZ_64M)
#define        NODES_SHIFT	2	/* 4 Nodes */
#define NODE_MAX_MEM_SHIFT	24	/* 16 MiB */
#elif (SDRAM_MEM_SIZE == SZ_32M)
#define        NODES_SHIFT	2	/* 4 Nodes */
#define NODE_MAX_MEM_SHIFT	23	/* 8 MiB */
#else
#error "Please, #Define SDRAM_MEM_SIZE "
#endif

#define MXC_NUMNODES	(1 << NODES_SHIFT)
#define NODE_MAX_MEM_SIZE	(1 << NODE_MAX_MEM_SHIFT)

#define SET_NODE(mi, nid) { \
	(mi)->bank[(nid)].start = PHYS_OFFSET + (nid) * NODE_MAX_MEM_SIZE; \
	(mi)->bank[(nid)].size =  NODE_MAX_MEM_SIZE; \
	(mi)->bank[(nid)].node = (nid); \
	node_set_online(nid); \
}

/*!
 * Given a kernel address, find the home node of the underlying memory.
 */
#define KVADDR_TO_NID(addr) \
	(((unsigned long)(addr) - PAGE_OFFSET) >> NODE_MAX_MEM_SHIFT)

/*!
 * Given a page frame number, convert it to a node id.
 */
#define PFN_TO_NID(pfn) \
	(((pfn) - PHYS_PFN_OFFSET) >> (NODE_MAX_MEM_SHIFT - PAGE_SHIFT))

/*!
 * Given a kaddr, ADDR_TO_MAPBASE finds the owning node of the memory
 * and returns the mem_map of that node.
 */
#define ADDR_TO_MAPBASE(kaddr) NODE_MEM_MAP(KVADDR_TO_NID(kaddr))

/*!
 * Given a page frame number, find the owning node of the memory
 * and returns the mem_map of that node.
 */
#define PFN_TO_MAPBASE(pfn)    NODE_MEM_MAP(PFN_TO_NID(pfn))

/*!
 * Given a kaddr, LOCAL_MAR_NR finds the owning node of the memory
 * and returns the index corresponding to the appropriate page in the
 * node's mem_map.
 */
#define LOCAL_MAP_NR(addr) \
	(((unsigned long)(addr) & (NODE_MAX_MEM_SIZE - 1)) >> PAGE_SHIFT)

#endif				/* CONFIG_DISCONTIGMEM */

#endif				/* __ASM_ARCH_MXC_MEMORY_H__ */
