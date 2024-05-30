/*----------------------------------------------------------------------------
 *               Video Buffer Pool Manager
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/spinlock.h>

#include "vbufpool.h"


//#undef DEBUG
//#define DEBUG

#ifdef DEBUG
static int debug = 10;
//MODULE_PARM(debug, "i");

#define dprint(lev, fmt, args...) if (debug >= lev) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define dprint(lev, fmt, args...) do {} while (0)
#endif

/*--------------------------------------------------------------------------*/

#define COMMAND_MAX 2000
#define OUTPUT_MAX  5000

#define SIMPLE_REMOVE	0
#define FORCE_REMOVE	1

/*--------------------------------------------------------------------------*/

#define KERN_COMMAND_LINE_MAX 200
static char kern_command_line[KERN_COMMAND_LINE_MAX] = { 0 };

static struct proc_dir_entry *proc_entry=NULL;

/*
 * To protect the vbuf pool data structures the following
 * spin lock is used. 
 *
 * One of the assumptions made when writing this code is that
 * The number of buffer managed by the pool will to small and
 * the number of calls to this interface will also be small.
 * So with these assumptions, one global spin lock is used.
 * The code will work, but performance will suffer, if these
 * assumptions are not true.
 *
 * Another locking assumption is, once a bucket structure has
 * been put in the pool list, it will not be removed until
 * the module is unloaded.
 */
static spinlock_t vbuf_lock = SPIN_LOCK_UNLOCKED;

struct buffer
{
	struct buffer *next;
	size_t         size;
	size_t         size_requested;
	void          *virt_addr;  
	unsigned long  dma_addr;
};

static struct buffer * buffer_pool=NULL;

static void * vbuf_virt_addr=NULL;
static unsigned long vbuf_dma_addr=0;
static size_t vbuf_size=0;


#define ADD_VBUF_POOL_CMD       1
#define REMOVE_VBUF_POOL_CMD    2
#define REMOVEALL_CMD 			3

struct vbuf_name_value verb_array[] = 
{
	{"add_pool_buffer", ADD_VBUF_POOL_CMD},
	{"remove_pool_buffer", REMOVE_VBUF_POOL_CMD},
	{"removeall", REMOVEALL_CMD},
	{0,0}
};

static struct buffer * new_buffer(void)
{
	struct buffer *z_pBuffer;

	z_pBuffer = (struct buffer *)kmalloc(sizeof(struct buffer), GFP_KERNEL);
	if (0 == z_pBuffer) {
		return 0;
	}	
	
	z_pBuffer->next = 0;
	z_pBuffer->size = 0;
	z_pBuffer->size_requested = 0;
	z_pBuffer->virt_addr = 0;
	z_pBuffer->dma_addr = 0;
	
	return z_pBuffer;
}

static int vbuf_pool_alloc(size_t size)
{
	if (vbuf_virt_addr!=NULL)
	{
		printk("[VBUFPOOL] - ERROR - buffer pool is already allocated\n");
		return -1;
	}

	vbuf_size = PAGE_ALIGN(size);
	vbuf_virt_addr = vbuf_pool_system_alloc(GFP_KERNEL|GFP_DMA, vbuf_size, (dma_addr_t *)(&vbuf_dma_addr), UNUSED_PARAM);
	if(0 == vbuf_virt_addr) {
		vbuf_size = 0;
		printk("[VBUFPOOL] - ERROR - not enough memory to allocate pool\n");
		return -1;	
	}

	return 0;
}

static void vbuf_pool_logical_free(void)
{
	struct buffer *z_pBuffer = buffer_pool;
	struct buffer *z_pBufferTmp;
	
	while(z_pBuffer) {
		z_pBufferTmp = z_pBuffer;
		z_pBuffer = z_pBuffer->next;
		
		kfree(z_pBufferTmp);
	}
	
	buffer_pool = 0;
}

static int vbuf_pool_free(void)
{
	if (vbuf_virt_addr==NULL)
	{
		printk("[VBUFPOOL] - ERROR - no buffer pool to free\n");
		return -1;
	}

	vbuf_pool_logical_free();
	
	vbuf_pool_system_free(vbuf_virt_addr, vbuf_size, vbuf_dma_addr);
	vbuf_virt_addr = 0;
	vbuf_size = 0;
	vbuf_dma_addr = 0;
	return 0;
}

/*----------------------------------------------------------------------------
 * vbuf_alloc
 *
 * Allocate memory for use with DMA. The memory will be physically contiguous
 * and in a range that can be reached by DMA. The DMA address of the memory
 * is returned through the dma_addr pointer. If the vbuf pool does not 
 * have a buffer that will satisfy the request then the request will be
 * passed on to the system.
 * 
 * Parameters:
 *    size      size of the memory in bytes
 *    type      must be one of the values defined in vbufpool.h
 *              It determines the flags that are used
 *              by the memory management hardware.
 *    dma_addr  a pointer used to return the DMA or physical address
 *              of the memory.
 * 
 * Return value is the kernel virtual address of the memory
 *        or NULL if error.
 *--------------------------------------------------------------------------*/
void * vbuf_alloc(size_t size_requested, unsigned int type, dma_addr_t *dma_addr)
{
	size_t size = PAGE_ALIGN(size_requested);
	struct buffer *	z_pBuffer=NULL;
	struct buffer * z_pNewBuffer=NULL;
	
	dprint(9,"[VBUFPOOL] %s:%d, ENTER\n",__FUNCTION__,__LINE__);
	
	if(vbuf_virt_addr) {	
		spin_lock(&vbuf_lock);
		if(0 == buffer_pool) {
			/* la pool est vide */
			dprint(9,"[VBUFPOOL] %s:%d, pool empty\n",__FUNCTION__,__LINE__);
			if(size <= (vbuf_size)) {
				z_pNewBuffer = new_buffer();
				if(0 == z_pNewBuffer) {
					printk("[VBUFPOOL] %s:%d, cannol allocate buffer structure\n",__FUNCTION__,__LINE__);
					return 0;
				}

				z_pNewBuffer->size = size;
				z_pNewBuffer->size_requested = size_requested;
				z_pNewBuffer->virt_addr = vbuf_virt_addr;
				z_pNewBuffer->dma_addr = vbuf_dma_addr;
				buffer_pool = z_pNewBuffer;
				
				if(dma_addr) {
					*dma_addr = z_pNewBuffer->dma_addr;
				}

				spin_unlock(&vbuf_lock);
				
				dprint(8,"[VBUFPOOL] %s:%d, insert au debut addr: 0x%x size:%d\n",__FUNCTION__,__LINE__,(unsigned int)z_pNewBuffer->dma_addr,z_pNewBuffer->size_requested);
			
				return z_pNewBuffer->virt_addr;
			}
			else {
				spin_unlock(&vbuf_lock);
				printk("[VBUFPOOL] %s:%d, buffer KO size too big size=%d>buffer pool size=%d\n",__FUNCTION__,__LINE__,size_requested,vbuf_size);
				return 0;
			}
		}
		else {
			/* au moins 1 element dans la pool */
			dprint(9,"[VBUFPOOL] %s:%d, 1 elem dans pool\n",__FUNCTION__,__LINE__);
			z_pBuffer = buffer_pool;
			if(z_pBuffer->virt_addr != vbuf_virt_addr) {
				/* place libre au debut */
				if((vbuf_virt_addr + size) < z_pBuffer->virt_addr) {
					z_pNewBuffer = new_buffer();
					if(0 == z_pNewBuffer) {
						printk("[VBUFPOOL] %s:%d, cannol allocate buffer structure\n",__FUNCTION__,__LINE__);
						return 0;
					}

					z_pNewBuffer->size = size;
					z_pNewBuffer->size_requested = size_requested;
					z_pNewBuffer->virt_addr = vbuf_virt_addr;
					z_pNewBuffer->dma_addr = vbuf_dma_addr;
					z_pNewBuffer->next = z_pBuffer;
					buffer_pool = z_pNewBuffer;				
					
					if(dma_addr) {
						*dma_addr = z_pNewBuffer->dma_addr;
					}
					spin_unlock(&vbuf_lock);
					dprint(8,"[VBUFPOOL] %s:%d, insert au debut addr:0x%x size:%d\n",__FUNCTION__,__LINE__,(unsigned int)z_pNewBuffer->dma_addr,z_pNewBuffer->size_requested);
					
					return z_pNewBuffer->virt_addr;
		
					/* espace libre OK ! */
				}
			}
			
			/* pas de place libre au debut ou insuffisant */
			dprint(9,"[VBUFPOOL] %s:%d, pas de place libre au debut ou insuffisant\n",__FUNCTION__,__LINE__);
			while(z_pBuffer->next && ((z_pBuffer->virt_addr + z_pBuffer->size + size) > z_pBuffer->next->virt_addr)) {
				z_pBuffer = z_pBuffer->next;
			}
			if(z_pBuffer->next) {
				z_pNewBuffer = new_buffer();
				if(0 == z_pNewBuffer) {
					printk("[VBUFPOOL] %s:%d, cannol allocate buffer structure\n",__FUNCTION__,__LINE__);
					return 0;
				}
				/* on insere au milieu */
				z_pNewBuffer->size = size;
				z_pNewBuffer->size_requested = size_requested;
				z_pNewBuffer->virt_addr = z_pBuffer->virt_addr + z_pBuffer->size;
				z_pNewBuffer->dma_addr = z_pBuffer->dma_addr + z_pBuffer->size;
				z_pNewBuffer->next = z_pBuffer->next;
				z_pBuffer->next = z_pNewBuffer;
				
				if(dma_addr) {
					*dma_addr = z_pNewBuffer->dma_addr;
				}
				
				spin_unlock(&vbuf_lock);
				
				dprint(8,"[VBUFPOOL] %s:%d, insert au milieu addr:0x%x size:%d\n",__FUNCTION__,__LINE__,(unsigned int)z_pNewBuffer->dma_addr,z_pNewBuffer->size_requested);
				
				return z_pNewBuffer->virt_addr;
			}
			else {
				/* on insere a la fin */
				dprint(9,"[VBUFPOOL] %s:%d, insert fin %lu / %lu\n",__FUNCTION__,__LINE__,(unsigned long)(z_pBuffer->virt_addr + z_pBuffer->size - vbuf_virt_addr + size),(unsigned long)(vbuf_size));
				if((z_pBuffer->virt_addr + z_pBuffer->size - vbuf_virt_addr + size) <= (vbuf_size)) {
					z_pNewBuffer = new_buffer();
					if(0 == z_pNewBuffer) {
						printk("[VBUFPOOL] %s:%d, cannol allocate buffer structure\n",__FUNCTION__,__LINE__);
						return 0;
					}

					z_pNewBuffer->size = size;
					z_pNewBuffer->size_requested = size_requested;
					z_pNewBuffer->virt_addr = z_pBuffer->virt_addr + z_pBuffer->size;
					z_pNewBuffer->dma_addr = z_pBuffer->dma_addr + z_pBuffer->size;
					z_pNewBuffer->next = z_pBuffer->next;
					z_pBuffer->next = z_pNewBuffer;
					
					if(dma_addr) {
						*dma_addr = z_pNewBuffer->dma_addr;
					}
					spin_unlock(&vbuf_lock);
					
					dprint(8,"[VBUFPOOL] %s, taille libre fin OK, addr:0x%x size:%d\n",__FUNCTION__,(unsigned int)z_pNewBuffer->dma_addr,z_pNewBuffer->size_requested);
					
					return z_pNewBuffer->virt_addr;
				}
				else {
					spin_unlock(&vbuf_lock);
					printk("[VBUFPOOL] %s:%d, taille libre fin KO !!\n",__FUNCTION__,__LINE__);		
					return 0;
				}
			}
		}

		spin_unlock(&vbuf_lock);
	}
	else
	{
#ifdef CONFIG_VBUFPOOL_DYNAMIC
		return dma_alloc_coherent(0, size_requested,dma_addr,GFP_DMA | GFP_KERNEL);
#else
		printk("[VBUFPOOL] %s:%d try to allocate in kernel memory pool that do not exist !!\n",__FUNCTION__,__LINE__);
#endif
	}
	return 0;
}

/*----------------------------------------------------------------------------
 * vbuf_free
 *
 * 
 * Parameters:
 *    vaddr     virtual address return by vbuf_alloc
 *    size      size of the memory in bytes, same as used with vbuf_alloc
 *    type      same as used with vbuf_alloc
 *    dma_addr  DMA or physical address that was returned by buf_alloc.
 *--------------------------------------------------------------------------*/
void
vbuf_free(void *vaddr, size_t size_requested, unsigned int type, dma_addr_t dma_addr)
{
	struct buffer *	z_pBuffer = NULL;
	struct buffer *	z_pBufferToRemove = NULL;

	spin_lock(&vbuf_lock);
	z_pBuffer=buffer_pool;
	if(z_pBuffer && z_pBuffer->size_requested == size_requested && z_pBuffer->dma_addr == dma_addr) {
		z_pBufferToRemove = z_pBuffer;
		buffer_pool = z_pBuffer->next;
	}
	else {
		if(z_pBuffer) {
			while(z_pBuffer->next && (z_pBuffer->next->size_requested != size_requested || z_pBuffer->next->dma_addr != dma_addr)) {
				z_pBuffer = z_pBuffer->next;
			}
			
			if(z_pBuffer->next && z_pBuffer->next->size_requested == size_requested && z_pBuffer->next->dma_addr == dma_addr) {
				z_pBufferToRemove = z_pBuffer->next;
				z_pBuffer->next = z_pBuffer->next->next;	
			}
		}
	}

	spin_unlock(&vbuf_lock);
	
	if(z_pBufferToRemove) {
		dprint(8,"[VBUFPOOL], free buffer addr:0x%x size:%d\n",(unsigned int)dma_addr,size_requested);
		kfree(z_pBufferToRemove);
	}
	else if (z_pBuffer)
	{
#ifdef CONFIG_VBUFPOOL_DYNAMIC
		dma_free_coherent(0, size_requested, vaddr,dma_addr);
#endif
		printk("[VBUFPOOL] cannot free buffer 0x%x of size=%d!!\n",(unsigned int)dma_addr,size_requested);
	}
}

/*----------------------------------------------------------------------------
 * vbuf_clean
 *
 * Used to clean the cache
 * 
 * Parameters:
 *    vaddr     virtual address return by vbuf_alloc
 *    size      size of the memory in bytes, same as used with vbuf_alloc
 *--------------------------------------------------------------------------*/

void vbuf_clean(void *vaddr, size_t size)
{
	dprint(9,KERN_ERR "[VBUFPOOL] %s\n",__FUNCTION__);
	vbuf_pool_system_clean_cache(vaddr, size);
}


/*----------------------------------------------------------------------------
 * vbuf_invalidate
 *
 * Used to invalidate the cache
 * 
 * Parameters:
 *    vaddr     virtual address return by vbuf_alloc
 *    size      size of the memory in bytes, same as used with vbuf_alloc
 *--------------------------------------------------------------------------*/

void vbuf_invalidate(void *vaddr, size_t size)
{
	dprint(9,KERN_ERR "[VBUFPOOL] %s\n",__FUNCTION__);
	vbuf_pool_system_inv_cache(vaddr, size);
}


/*----------------------------------------------------------------------------
 * Skip white space.
 *
 * Returns a pointer to character after any white space.
 * White space includes 'space', tab and any control characters.
 *--------------------------------------------------------------------------*/
static char *
skip_white_space(char *cmd)
{
	while (*cmd <= ' ' && *cmd != 0) {
		dprint(9, "Skipping white space\n");
		cmd++;
	}
	return cmd;
}

/*----------------------------------------------------------------------------
 * get the verb from the command line
 *
 * return 0 on success and 1 on error
 *--------------------------------------------------------------------------*/
int
get_value(char **cmd, struct vbuf_name_value *list, unsigned int *value)
{
	struct vbuf_name_value *pair_def;
	int len;

	dprint(5, "get_value: addr: %08X, cmd: (%s)\n", (int) *cmd, *cmd);
	pair_def = list;
	while (pair_def->name)
	{
		len = strlen(pair_def->name);
		if (strnicmp(pair_def->name, *cmd, len) == 0) {
			dprint(9, "command: %s, len: %d\n", *cmd, len);
			if ((*cmd)[len] == ':') {
				len++;
			}
			*value = pair_def->value;
			*cmd  += len;
			return 0;
		}
		pair_def++;
	}
	return 1;
}

/*----------------------------------------------------------------------------
 * get a number from the command line
 * 
 * return value is 0 on success and 1 on error
 *--------------------------------------------------------------------------*/
static int
get_number(char **cmd, unsigned long *value)
{
	char *endp;
	unsigned long num;
	
	num = simple_strtoul(*cmd, &endp, 0);
	if (*cmd == endp) {
		return 1;
	}
	switch (endp[0]) {
	case 'k':
	case 'K':
		num <<= 10;
		endp++;
		break;

	case 'm':
	case 'M':
		num <<= 20;
		endp++;
		break;
	
	case 'g':
	case 'G':
		num <<= 30;
		endp++;
		break;
	
	default:
		break;
	}
	
	*cmd = endp;
	*value = num;
	
	return 0;
}

static int get_vbuf_size(char **cmd, size_t *size)
{
	unsigned long num;
	char *cur;
	
	cur  = *cmd;
	
	if (get_number(&cur, &num)) {
		return 1;
	}
	cur = skip_white_space(cur);
	
	*size  = num;
	*cmd = cur;
	return 0;
}
/*----------------------------------------------------------------------------
 * parse and execute a command
 *--------------------------------------------------------------------------*/
static int
do_command(char *cmd)
{
	char  *cur;
	size_t size;
	unsigned int verb;
	
	dprint(9, "[VBUFPOOL] - do_command beginning\n");
	cur = skip_white_space(cmd);
	if (get_value(&cur, verb_array, &verb)) {
		printk("[VBUFPOOL] - ERROR - unknow command\n");
		return -EINVAL;
	}
	switch (verb) {
	case ADD_VBUF_POOL_CMD:
		if (get_vbuf_size(&cur, &size)) {
			printk("[VBUFPOOL] - ERROR - bad command line\n");
			return -EINVAL;
		}
		spin_lock(&vbuf_lock);
		vbuf_pool_alloc(size);
		spin_unlock(&vbuf_lock);
		break;
	case REMOVE_VBUF_POOL_CMD:
		//remove pool of buffer
		spin_lock(&vbuf_lock);
		vbuf_pool_free();
		spin_unlock(&vbuf_lock);
		break;
	case REMOVEALL_CMD:
		//remove buffer
		spin_lock(&vbuf_lock);
		vbuf_pool_logical_free();
		spin_unlock(&vbuf_lock);
		break;
	default:
		break;
	}
	
	return cur - cmd;
}

/*----------------------------------------------------------------------------
 * read /proc/vbufpool
 *
 * A read from the proc file will return the current status of the
 * vbuf pool. It has two sections. 
 * 
 * The first shows the current state of the
 * buffer pool. For each sized buffer bucket it shows the following
 *    buf-size  - is the size in bytes of these buffers.
 *    type      - the type of these buffers.
 *    used      - is the number of buffers being used.
 *    free      - is the number of buffers in the pool ready for allocation
 *    total     - number of buffers of this size and type being managed by the pool.
 *    mem-total - amount of memory in bytes being managed by the pool.
 *        The last number is the total amount of memory in bytes being
 *        managed by the pool.
 * 
 * The second section show statistics on all allocations. This includes memory
 * being managed by the pool and memory allocated from the system.
 *    buf-size  - is the size in bytes of these buffers
 *    type      - the type of these buffers
 *    peak used - peak number of buffers ever in use
 *    hit       - number of allocations from the pool
 *    miss      - number of allocations from general system memory 
 *    total     - total number of allocations for this size and type of memory 
 *    
 * These statistics can be used to determine the size and quantity of buffers
 * that should be pre-allocated. Without pre-allocating any buffers run your
 * application. Now look at the vbufpool statistics.
 *--------------------------------------------------------------------------*/
static int
proc_read(char *page, char **start, off_t off, int count,
		 int *eof, void *data)
{
	int len;
	char output[OUTPUT_MAX];
	struct buffer *	z_pBuffer = buffer_pool;
	long total_size = 0;
	long size;
	len = 0;
	
	if(vbuf_virt_addr) {
		len += snprintf(output+len, OUTPUT_MAX-len, "   VBUFPOOL usage\n");
		
		if(!z_pBuffer) {
			size = (long)vbuf_size;
			total_size += size;
			len += snprintf(output+len, OUTPUT_MAX-len,"free space      %10ld\n",size);
		}
		
		if(z_pBuffer && z_pBuffer->virt_addr != vbuf_virt_addr) {
			size = (long)(z_pBuffer->virt_addr - vbuf_virt_addr);
			total_size += size;
			len += snprintf(output+len, OUTPUT_MAX-len,"free space  %10ld\n",size);
		}
		while(z_pBuffer) {
			size = (long)z_pBuffer->size;
			total_size +=size;
			len += snprintf(output+len, OUTPUT_MAX-len,"0x%8x      %10ld\n",(unsigned int)z_pBuffer->virt_addr,size);
			if(z_pBuffer->next && (z_pBuffer->virt_addr + z_pBuffer->size != z_pBuffer->next->virt_addr)) {
				size = (long)z_pBuffer->next->virt_addr - ((long)z_pBuffer->virt_addr + z_pBuffer->size);
				total_size +=size;
				len += snprintf(output+len, OUTPUT_MAX-len,"free space      %10ld\n",size);
			}
			if(!z_pBuffer->next) {
				size = vbuf_size - ((long)z_pBuffer->virt_addr + z_pBuffer->size - (long)vbuf_virt_addr);
				total_size +=size;
				len += snprintf(output+len, OUTPUT_MAX-len,"free space      %10ld\n",size);
			}
			z_pBuffer = z_pBuffer->next;
		}
		
		len += snprintf(output+len, OUTPUT_MAX-len, "total size      %10ld (should be %ld)\n\n",total_size,(long)vbuf_size);
	}
	else {
		len += snprintf(output+len, OUTPUT_MAX-len, "Vbufpool buffer isn't allocated\n\n");
	}	
	
	len += snprintf(output+len,OUTPUT_MAX-len,"commands:\n");
	len += snprintf(output+len,OUTPUT_MAX-len,
		"echo add_pool_buffer:SIZE > /proc/vbufpool : create pool of size SIZE bytes\n");
	len += snprintf(output+len,OUTPUT_MAX-len,
		"echo removeall > /proc/vbufpool            : remove all buffers\n");
	len += snprintf(output+len,OUTPUT_MAX-len,
		"echo remove_pool_buffer > /proc/vbufpool   : remove all buffers and pool\n\n");

	if (len > count) {
		len = count;
	} else {
		*eof = 1;
	}

	memcpy(page, output, len);
	return len;
}

/*----------------------------------------------------------------------------
 * write /proc/vbufpool
 * 
 * You can write a command to this proc file to setup and manage the
 * vbufpool. The commands are of the following form.
 *    command:buf-spec[,buf-spec]
 * A command followed by one or more buffer specifications.
 * The buf-spec has the following format.
 *    <count>x<size>[-<type>]
 * A count then an 'x' followed by the size and optionally followed by
 * the type of memory. Count is the number of buffers. The size can use
 * modifiers like 'K', 'M', and 'G'. The type can be one of the following:
 *   nc  non-cacheable (the default)
 *   c   cacheable
 *   b   bufferable
 * For example, to add buffers to the buffer pool you could use this command
 *    echo add:3x140k-c,5x30k > /proc/vbufpool
 * This adds to the pool three 140K byte buffers and five 30K byte buffers.
 * The three 140k buffers are type 'cacheable'.
 * You can remove buffers with this command.
 *    echo remove:1x140k,2x30k > /proc/vbufpool
 * This removes one of the 140K and two of the 30K buffers from the pool
 * There is one more command to remove all of the buffers from the pool.
 *    echo removeall > /proc/vbufpool
 *--------------------------------------------------------------------------*/
static int 
proc_write(struct file* file, const char* buffer, 
         unsigned long count, void* data)
{
	char cmd[COMMAND_MAX];
	int ret;
	
	if (count >= COMMAND_MAX) {
		return -E2BIG;
	}
	
	if(copy_from_user(cmd, buffer, count)) {
		return -EFAULT;
	}
	
	cmd[count] = 0;

	dprint(7, "cmd = (%s)\n", cmd);
	if(strlen(cmd) <= 2) { // This cannot be a command (too short) DIRTY FIX
		dprint(9, "Empty command\n");
		ret = count;
	} else {	
		ret =  do_command(cmd);
	}
	dprint(7, "ret = %d\n", ret);

	return ret;
}

#ifndef MODULE
/*----------------------------------------------------------------------------
 * handle the vbufpool kernel command line option
 *--------------------------------------------------------------------------*/
static int __init vbufpool_setup(char *str)
{
	int len;
	dprint(9, "Running in the kernel: vbufpool_setup kernel init\n");
	len = strpbrk(str, " \n\r\t") - str;
	if (len >= KERN_COMMAND_LINE_MAX) {
		len = KERN_COMMAND_LINE_MAX;
	}
	strncpy(kern_command_line, str, len);
	
	return 1;
}
__setup("vbufpool=", vbufpool_setup);
#endif

/*----------------------------------------------------------------------------
 * Initialization code
 *
 * Run only once. 
 *--------------------------------------------------------------------------*/
static int __init
vbufpool_init(void)
{
	char *cmd;
	
	dprint(9, "vbufpool_init:\n");

	proc_entry = create_proc_entry("vbufpool", 0644, NULL);
	if (proc_entry) {
		proc_entry->write_proc = proc_write;
		proc_entry->read_proc  = proc_read;
		proc_entry->data       = NULL;
	}
	
	vbuf_virt_addr = 0;
	vbuf_dma_addr = 0;
	vbuf_size = 0;

#ifndef MODULE
	dprint(9, "Running in kernel\n");
	cmd = kern_command_line;
#else
	dprint(9, "Running as module\n");
	cmd = command;
#endif
	
#ifdef CONFIG_VBUFPOOL_INIT_ALLOC_SIZE
	spin_lock(&vbuf_lock);
	vbuf_pool_alloc(CONFIG_VBUFPOOL_INIT_ALLOC_SIZE * 1024);
	spin_unlock(&vbuf_lock);
#endif
	
	return 0;
}


/*----------------------------------------------------------------------------
 * Exit code.
 *--------------------------------------------------------------------------*/
static void __exit
vbufpool_exit(void)
{
	
	if (proc_entry) {
		remove_proc_entry("vbufpool", NULL);
		proc_entry = NULL;
	}
}

EXPORT_SYMBOL(vbuf_alloc);
EXPORT_SYMBOL(vbuf_free);
EXPORT_SYMBOL(vbuf_clean);
EXPORT_SYMBOL(vbuf_invalidate);

MODULE_AUTHOR ("Guillaume CHAUVEL");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION ("buffer pool manager");

module_init(vbufpool_init);
module_exit(vbufpool_exit);

