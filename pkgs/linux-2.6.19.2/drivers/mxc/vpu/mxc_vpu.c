/*
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 06/02/2008 
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

/*!
 * @file mxc_vpu.c
 *
 * @brief VPU system initialization and file operation implementation
 *
 * @ingroup VPU
 */

#ifdef	CONFIG_MXC_VPU_DEBUG
#define	DEBUG
#include <linux/kernel.h>
#endif
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/autoconf.h>
#include <linux/ioport.h>
#include <linux/stat.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <asm/arch/clock.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <asm/dma-mapping.h>
#include <asm/hardware.h>

#include <asm/arch/mxc_vpu.h>

#define	BIT_INT_CLEAR		0x00C
#define	BIT_INT_STATUS		0x010
#define BIT_INT_ENABLE		0x170

struct vpu {
	struct fasync_struct *async_queue;
};

/* To track the allocated memory buffer */
struct memalloc_record {
	struct list_head list;
	vpu_mem_desc mem;
};

static DEFINE_SPINLOCK(vpu_lock);
static LIST_HEAD(head);

static int vpu_major = 0;
static struct class *vpu_class;
static struct vpu vpu_data;
static u8 open_count = 0;
static struct clk *vpu_clk;

/* implement the blocking ioctl */
static int codec_done = 0;
static wait_queue_head_t vpu_queue;
static int wait_intr_cnt = 0;

/*!
 * Private function to free buffers
 * @return status  0 success.
 */
static int vpu_free_buffers(void)
{
	struct memalloc_record *rec, *n;
	vpu_mem_desc mem;
	unsigned long flags;

	spin_lock_irqsave(&vpu_lock, flags);
	list_for_each_entry_safe(rec, n, &head, list) {
		mem = rec->mem;
		if (mem.cpu_addr != 0) {
#ifdef CONFIG_VBUFPOOL
			vbuf_free((void *)mem.cpu_addr, PAGE_ALIGN(mem.size), VBUF_TYPE_NON_CACHED, mem.phy_addr);
			pr_debug("%s - vbuf_free paddr=0x%08X size=%d\n", __FUNCTION__,(unsigned int)mem.phy_addr,(int)PAGE_ALIGN(mem.size));
			mem.phy_addr = 0;
			mem.cpu_addr = 0;
			list_del(&rec->list);
#else
			dma_free_coherent(0, PAGE_ALIGN(mem.size),
					  (void *)mem.cpu_addr, mem.phy_addr);
			pr_debug("%s - dma_free_coherent paddr=0x%08X size=%d\n", __FUNCTION__,(unsigned int)mem.phy_addr,(int)PAGE_ALIGN(mem.size));
			mem.phy_addr = 0;
			mem.cpu_addr = 0;
			list_del(&rec->list);
#endif
			kfree(rec);
		}
	}
	spin_unlock_irqrestore(&vpu_lock, flags);

	return 0;
}

/*!
 * @brief vpu interrupt handler
 */
static irqreturn_t vpu_irq_handler(int irq, void *dev_id)
{
	struct vpu *dev;
	dev = (struct vpu *)dev_id;
	__raw_readl(IO_ADDRESS(VPU_BASE_ADDR + BIT_INT_STATUS));
	__raw_writel(0x1, IO_ADDRESS(VPU_BASE_ADDR + BIT_INT_CLEAR));
	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

	codec_done = 1;
	wake_up_interruptible(&vpu_queue);

	return IRQ_HANDLED;
}

/*!
 * @brief vpu hardware enable function
 *
 * @return  0 on success or negative error code on error
 */
static int vpu_hardware_enable(void)
{
	clk_enable(vpu_clk);
	/* enable user space access for vpu register */
	__raw_writel(0x1, IO_ADDRESS(AIPI_BASE_ADDR + 0x20008));
	return 0;
}

/*!
 * @brief vpu hardware disable function
 *
 * @return  0 on success or negative error code on error
 */
static int vpu_hardware_disable(void)
{
	clk_disable(vpu_clk);
	__raw_writel(0xffffffff, IO_ADDRESS(AIPI_BASE_ADDR + 0x20008));
	return 0;

}

/*!
 * @brief open function for vpu file operation
 *
 * @return  0 on success or negative error code on error
 */
static int vpu_open(struct inode *inode, struct file *filp)
{
	if (open_count++ == 0) {
		filp->private_data = (void *)(&vpu_data);
		vpu_hardware_enable();
	} else {
		printk(KERN_ERR "VPU has already been opened.\n");
		return -EACCES;
	}

	return 0;
}

/*!
 * @brief IO ctrl function for vpu file operation
 * @param cmd IO ctrl command
 * @return  0 on success or negative error code on error
 */
static int vpu_ioctl(struct inode *inode, struct file *filp, u_int cmd,
		     u_long arg)
{
	unsigned long flags;
	int ret = 0;

	switch (cmd) {
	case VPU_IOC_PHYMEM_ALLOC:
		{
			struct memalloc_record *rec;

			rec = kzalloc(sizeof(*rec), GFP_KERNEL);
			if (!rec)
			{
				printk("%s - ERROR - kzalloc failed\n",__FUNCTION__);
				return -ENOMEM;
			}

			if (copy_from_user(&(rec->mem), (vpu_mem_desc *)arg,
					   sizeof(vpu_mem_desc))) {
				kfree(rec);
				printk("%s - ERROR - copy_from_user failed\n",__FUNCTION__);
				return -EFAULT;
			}
			pr_debug("%s - mem alloc size = 0x%x\n", __FUNCTION__,
				 rec->mem.size);
#ifdef CONFIG_VBUFPOOL
			rec->mem.cpu_addr = (unsigned long) vbuf_alloc(PAGE_ALIGN(rec->mem.size), VBUF_TYPE_NON_CACHED, (dma_addr_t
						*) (&(rec->mem.phy_addr)));
			pr_debug("VPU_IOC_PHYMEM_ALLOC - vbuf_alloc addr = 0x%x size=%d\n",
				 (unsigned int)rec->mem.phy_addr,(int)PAGE_ALIGN(rec->mem.size));
#else
			rec->mem.cpu_addr = (unsigned long)
			    dma_alloc_coherent(NULL,
					       PAGE_ALIGN(rec->mem.size),
					       (dma_addr_t
						*) (&(rec->mem.phy_addr)),
					       GFP_DMA | GFP_KERNEL);
			pr_debug("VPU_IOC_PHYMEM_ALLOC - dma_alloc_coherent addr = 0x%x size=%d\n",
				 (unsigned int)rec->mem.phy_addr,(int)PAGE_ALIGN(rec->mem.size));
#endif
			if ((void *)(rec->mem.cpu_addr) == NULL) {
				kfree(rec);
				printk(KERN_ERR
				       "Physical memory allocation error!\n");
				ret = -ENOMEM;
				break;
			}
			if (copy_to_user((void __user *)arg, &(rec->mem),
					 sizeof(vpu_mem_desc))) {
				kfree(rec);
				printk("%s - ERROR - copy_to_user failed\n",__FUNCTION__);
				return -EFAULT;
			}

			spin_lock_irqsave(&vpu_lock, flags);
			list_add(&rec->list, &head);
			spin_unlock_irqrestore(&vpu_lock, flags);

			break;
		}
	case VPU_IOC_PHYMEM_FREE:
		{
			struct memalloc_record *rec, *n;
			vpu_mem_desc vpu_mem;

			if (copy_from_user(&vpu_mem, (vpu_mem_desc *)arg,
					   sizeof(vpu_mem_desc))) {
				return -EFAULT;
			}
			pr_debug("%s - mem freed cpu_addr = 0x%x\n",__FUNCTION__,
				 vpu_mem.cpu_addr);

			spin_lock_irqsave(&vpu_lock, flags);
			list_for_each_entry_safe(rec, n, &head, list) {
				if (rec->mem.cpu_addr == vpu_mem.cpu_addr) {
#ifdef CONFIG_VBUFPOOL
			pr_debug("VPU_IOC_PHYMEM_FREE - vbuf_free - size=%d adrr=0x%x\n",(int)PAGE_ALIGN(vpu_mem.size),(unsigned int)vpu_mem.phy_addr);
			vbuf_free((void *)vpu_mem.cpu_addr, PAGE_ALIGN(vpu_mem.size), VBUF_TYPE_NON_CACHED, (dma_addr_t) vpu_mem.phy_addr);
#else
			pr_debug("VPU_IOC_PHYMEM_FREE - dma_free_coherent - size=%d adrr=0x%x\n",(int)PAGE_ALIGN(vpu_mem.size),(unsigned int)vpu_mem.phy_addr);
					dma_free_coherent(NULL,
							  PAGE_ALIGN(vpu_mem.size),
							  (void *)vpu_mem.cpu_addr,
							  (dma_addr_t) vpu_mem.
							  phy_addr);
#endif

					/* delete from list */
					list_del(&rec->list);
					kfree(rec);
					break;
				}
			}
			spin_unlock_irqrestore(&vpu_lock, flags);

			break;
		}
	case VPU_IOC_WAIT4INT:
		{
			u_long timeout = (u_long) arg;
			if (!wait_event_interruptible_timeout
			    (vpu_queue, codec_done != 0,
			     msecs_to_jiffies(timeout))) {
				printk(KERN_WARNING "VPU blocking: timeout.\n");
				ret = -ETIME;
				return ret;
			} else if (signal_pending(current)) {
				if (wait_intr_cnt == 0) {
					printk(KERN_WARNING "VPU interrupt received.\n");
				}
				wait_intr_cnt++;
				ret = -ERESTARTSYS;
				return ret;
			}

			codec_done = 0;
			break;
		}
		/* set/clear LHD (Latency Hiding Disable) bit in ESDCFG0 reg. 
		   Tends to fix MPEG4 issue on MX27 TO2 */
	case VPU_IOC_LHD:
		{
			u_int disable = (u_int) arg;
			u_int reg;
			u_int reg_addr;

			reg_addr = IO_ADDRESS(SDRAMC_BASE_ADDR + 0x10);
			reg = __raw_readl(reg_addr);
			pr_debug("ESDCFG0: [ 0x%08x ]\n", reg);

			if (disable == 0) {
				__raw_writel(reg & ~0x00000020, reg_addr);
				pr_debug("Latency Hiding Disable\n");
			} else {
				__raw_writel(reg | 0x00000020, reg_addr);
				pr_debug("Latency Hiding Enable\n");
			}

			pr_debug("ESDCFG0: [ 0x%08x ]\n",
				 __raw_readl(reg_addr));

			break;
		}
	case VPU_IOC_REG_DUMP:
		break;
	case VPU_IOC_PHYMEM_DUMP:
		break;
	default:
		{
			printk(KERN_ERR "No such IOCTL, cmd is %d\n", cmd);
			break;
		}
	}
	return ret;
}

/*!
 * @brief Release function for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_release(struct inode *inode, struct file *filp)
{
	pr_debug("%s - Enter\n",__FUNCTION__);

	if (--open_count == 0) {
		__raw_writel(0x0, IO_ADDRESS(VPU_BASE_ADDR + BIT_INT_ENABLE));
		vpu_free_buffers();
		vpu_hardware_disable();
	}

	return 0;
}

/*!
 * @brief fasync function for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_fasync(int fd, struct file *filp, int mode)
{
	struct vpu *dev = (struct vpu *)filp->private_data;
	pr_debug("%s - Enter\n",__FUNCTION__);
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}

/*!
 * @brief memory map function of harware registers for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_map_hwregs(struct file *fp, struct vm_area_struct *vm)
{
	unsigned long pfn;
	pr_debug("%s - Enter\n",__FUNCTION__);

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = VPU_BASE_ADDR >> PAGE_SHIFT;
	pr_debug("size=0x%x,  page no.=0x%x\n",
		 (int)(vm->vm_end - vm->vm_start), (int)pfn);
	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end - vm->vm_start,
			       vm->vm_page_prot) ? -EAGAIN : 0;
}

/*!
 * @brief memory map function of memory for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_map_mem(struct file *fp, struct vm_area_struct *vm)
{
	int request_size;
	request_size = vm->vm_end - vm->vm_start;

	pr_debug(" start=0x%x, pgoff=0x%x, size=0x%x\n",
		 (unsigned int)(vm->vm_start), (unsigned int)(vm->vm_pgoff),
		 request_size);

	vm->vm_flags |= VM_IO | VM_RESERVED;
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       request_size, vm->vm_page_prot) ? -EAGAIN : 0;

}

/*!
 * @brief memory map interface for vpu file operation
 * @return  0 on success or negative error code on error
 */
static int vpu_mmap(struct file *fp, struct vm_area_struct *vm)
{
	pr_debug("%s - Enter\n",__FUNCTION__);

	if (vm->vm_pgoff)
		return vpu_map_mem(fp, vm);
	else
		return vpu_map_hwregs(fp, vm);
}

struct file_operations vpu_fops = {
	.owner = THIS_MODULE,
	.open = vpu_open,
	.ioctl = vpu_ioctl,
	.release = vpu_release,
	.fasync = vpu_fasync,
	.mmap = vpu_mmap,
};

/*!
 * This function is called by the driver framework to initialize the vpu device.
 * @param   dev The device structure for the vpu passed in by the framework.
 * @return   0 on success or negative error code on error
 */
static int vpu_dev_probe(struct platform_device *pdev)
{
	int err = 0;
	struct class_device *temp_class;

	vpu_major = register_chrdev(vpu_major, "mxc_vpu", &vpu_fops);
	if (vpu_major < 0) {
		printk(KERN_ERR "vpu: unable to get a major for VPU\n");
		err = -EBUSY;
		return vpu_major;
	}

	vpu_class = class_create(THIS_MODULE, "mxc_vpu");
	if (IS_ERR(vpu_class)) {
		err = PTR_ERR(vpu_class);
		goto err_out_chrdev;
	}

	temp_class = class_device_create(vpu_class, NULL,
					 MKDEV(vpu_major, 0), NULL, "mxc_vpu");
	if (IS_ERR(temp_class)) {
		err = PTR_ERR(temp_class);
		goto err_out_class;
	}

	vpu_clk = clk_get(&pdev->dev, "vpu_clk");

	if (request_irq(INT_VPU, vpu_irq_handler, 0, "VPU_CODEC_IRQ",(void *)(&vpu_data))!=0)
	{
		panic("VPU: Could not allocate INT_VPU IRQ(%d)!\n", INT_VPU);
	}

	err = 0;
	goto out;

      err_out_class:
	printk("%s - ERROR - Line %d\n",__FUNCTION__,__LINE__);
	class_device_destroy(vpu_class, MKDEV(vpu_major, 0));
	class_destroy(vpu_class);
      err_out_chrdev:
	printk("%s - ERROR - Line %d\n",__FUNCTION__,__LINE__);
	unregister_chrdev(vpu_major, "mxc_vpu");
      out:
	return err;
}

/*! Driver definition
 *
 */
static struct platform_driver mxcvpu_driver = {
	.driver = {
		   .name = "mxc_vpu",
		   },
	.probe = vpu_dev_probe,
};

static int __init vpu_init(void)
{
	int ret = platform_driver_register(&mxcvpu_driver);
#ifdef CONFIG_MACH_MX27MEDIAPHONE
	struct clk *vpu_clk=NULL;
#endif

	pr_debug("%s - Enter\n",__FUNCTION__);
#ifdef CONFIG_MACH_MX27MEDIAPHONE
	//configure vpu clock
	vpu_clk = clk_get(NULL, "vpu_clk");
	vpu_clk->set_rate(vpu_clk,133000000);
#endif

	init_waitqueue_head(&vpu_queue);
	

	return ret;
}

static void __exit vpu_exit(void)
{
	pr_debug("%s - Enter\n",__FUNCTION__);

	free_irq(INT_VPU, (void *)(&vpu_data));
	if (vpu_major > 0) {
		class_device_destroy(vpu_class, MKDEV(vpu_major, 0));
		class_destroy(vpu_class);
		if (unregister_chrdev(vpu_major, "mxc_vpu") < 0) {
			printk(KERN_ERR
			       "Failed to unregister vpu from devfs\n");
			return;
		}
		vpu_major = 0;
	}

	clk_put(vpu_clk);

	platform_driver_unregister(&mxcvpu_driver);
	return;
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Linux VPU driver for Freescale i.MX27");
MODULE_LICENSE("GPL");

module_init(vpu_init);
module_exit(vpu_exit);
