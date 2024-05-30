/*
 * $Id: redboot.c,v 1.21 2006/03/30 18:34:37 bjd Exp $
 *
 * Parse RedBoot-style Flash Image System (FIS) tables and
 * produce a Linux partition array to match.
 *
 * Modified by Sagemcom under GPL license on 07/04/2009
 * Copyright (c) 2010 Sagemcom All rights reserved:
 * - Init undefined variable ret and l_status_block from driver mtd
 * - Redboot : Try to find the last good block
 * - Compute offset value with the last good block addr
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/vmalloc.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#define MAX_BAD_BLOCK	 0xFF

struct fis_image_desc {
    unsigned char name[16];      // Null terminated name
    uint32_t	  flash_base;    // Address within FLASH of image
    uint32_t	  mem_base;      // Address in memory where it executes
    uint32_t	  size;          // Length of image
    uint32_t	  entry_point;   // Execution entry point
    uint32_t	  data_length;   // Length of actual data
    uint32_t	  phy_address;   // Physical address of partition
    unsigned char _pad[256-(16+8*sizeof(uint32_t))];
    uint32_t	  desc_cksum;    // Checksum over image descriptor
    uint32_t	  file_cksum;    // Checksum over image data
};

struct fis_list {
	struct fis_image_desc *img;
	struct fis_list *next;
};

static int directory = CONFIG_MTD_REDBOOT_DIRECTORY_BLOCK;
module_param(directory, int, 0);

static inline int redboot_checksum(struct fis_image_desc *img)
{
	/* RedBoot doesn't actually write the desc_cksum field yet AFAICT */
	return 1;
}

static int parse_redboot_partitions(struct mtd_info *master,
                             struct mtd_partition **pparts,
                             unsigned long fis_origin)
{
	int nrparts = 0;
	struct fis_image_desc *buf;
	struct mtd_partition *parts;
	struct fis_list *fl = NULL, *tmp_fl;
	int i;
	size_t retlen;
	char *names;
	char *nullname;
	int namelen = 0;
	int nulllen = 0;
        int ret = 0;
        int l_status_block = -1;
	int numslots;
	unsigned long offset;
#ifdef CONFIG_MTD_REDBOOT_PARTS_UNALLOCATED
	static char nullstring[] = "unallocated";
#endif

	buf = vmalloc(master->erasesize);

	if (!buf)
		return -ENOMEM;

        if ( directory < 0 )
        {
           //Try to find the last good block
           for (i=1; (i<MAX_BAD_BLOCK) && (l_status_block!=0); i++)
           {
              offset = master->size - i*master->erasesize;
              l_status_block = master->block_isbad (master, offset); 
           }
           if (l_status_block != 0)
              goto out;
        }
        else
        {
           offset = directory*master->erasesize;
        }
	printk(KERN_NOTICE "Searching for RedBoot partition table in %s at offset 0x%lx\n",
	       master->name, offset);

	ret = master->read(master, offset,
			   master->erasesize, &retlen, (void *)buf);

	if (ret)
		goto out;

	if (retlen != master->erasesize) {
		ret = -EIO;
		goto out;
	}

	numslots = (master->erasesize / sizeof(struct fis_image_desc));
	for (i = 0; i < numslots; i++) {
		if (!memcmp(buf[i].name, "FIS directory", 14)) {
			/* This is apparently the FIS directory entry for the
			 * FIS directory itself.  The FIS directory size is
			 * one erase block; if the buf[i].size field is
			 * swab32(erasesize) then we know we are looking at
			 * a byte swapped FIS directory - swap all the entries!
			 * (NOTE: this is 'size' not 'data_length'; size is
			 * the full size of the entry.)
			 */
			if (swab32(buf[i].size) == master->erasesize) {
				int j;
				for (j = 0; j < numslots && buf[j].name[0] != 0xff; ++j) {
					/* The unsigned long fields were written with the
					 * wrong byte sex, name and pad have no byte sex.
					 */
					swab32s(&buf[j].flash_base);
					swab32s(&buf[j].phy_address);
					swab32s(&buf[j].mem_base);
					swab32s(&buf[j].size);
					swab32s(&buf[j].entry_point);
					swab32s(&buf[j].data_length);
					swab32s(&buf[j].desc_cksum);
					swab32s(&buf[j].file_cksum);
					
					if ( ! buf[j].phy_address ) {
						buf[j].phy_address = buf[j].flash_base ;
					}
				}
			}
			break;
		}
	}
	if (i == numslots) {
		/* Didn't find it */
		printk(KERN_NOTICE "No RedBoot partition table detected in %s\n",
		       master->name);
		ret = 0;
		goto out;
	}

	for (i = 0; i < numslots; i++) {
		struct fis_list *new_fl, **prev;

		/* Stop if we run into the Redboot Config partition */
		if (((uint32_t *) (buf + i))[1] == 0x0badface ||
		    ((uint32_t *) (buf + i))[1] == 0xcefaad0b)
			break;

		if (buf[i].name[0] == 0xff)
			continue;
		if (!redboot_checksum(&buf[i]))
			break;

		new_fl = kmalloc(sizeof(struct fis_list), GFP_KERNEL);
		namelen += strlen(buf[i].name)+1;
		if (!new_fl) {
			ret = -ENOMEM;
			goto out;
		}
		new_fl->img = &buf[i];
                if (fis_origin) {
                        buf[i].flash_base -= fis_origin;
                } else {
                        buf[i].flash_base &= master->size-1;
                }

		/* I'm sure the JFFS2 code has done me permanent damage.
		 * I now think the following is _normal_
		 */
		prev = &fl;
		while(*prev && (*prev)->img->flash_base < new_fl->img->flash_base)
			prev = &(*prev)->next;
		new_fl->next = *prev;
		*prev = new_fl;

		nrparts++;
	}
#ifdef CONFIG_MTD_REDBOOT_PARTS_UNALLOCATED
	if (fl->img->flash_base) {
		nrparts++;
		nulllen = sizeof(nullstring);
	}

	for (tmp_fl = fl; tmp_fl->next; tmp_fl = tmp_fl->next) {
		if (tmp_fl->img->flash_base + tmp_fl->img->size + master->erasesize <= tmp_fl->next->img->flash_base) {
			nrparts++;
			nulllen = sizeof(nullstring);
		}
	}
#endif
	parts = kmalloc(sizeof(*parts)*nrparts + nulllen + namelen, GFP_KERNEL);

	if (!parts) {
		ret = -ENOMEM;
		goto out;
	}

	memset(parts, 0, sizeof(*parts)*nrparts + nulllen + namelen);

	nullname = (char *)&parts[nrparts];
#ifdef CONFIG_MTD_REDBOOT_PARTS_UNALLOCATED
	if (nulllen > 0) {
		strcpy(nullname, nullstring);
	}
#endif
	names = nullname + nulllen;

	i=0;

#ifdef CONFIG_MTD_REDBOOT_PARTS_PHYSICAL
  /* new redboot method : use physical address 
   * TODO: give more delails.
   */
#define pl_flash_base   phy_address
#else
#error SAGEM: using logical addresses is not reliable !!
  /* old redboot fashion : use logical (aka linear) address 
   */
#define pl_flash_base   flash_base
#endif


#ifdef CONFIG_MTD_REDBOOT_PARTS_UNALLOCATED
	if (fl->img->pl_flash_base) {
	       parts[0].name = nullname;
	       parts[0].size = fl->img->pl_flash_base;
	       parts[0].offset = 0;
		i++;
	}
#endif
	for ( ; i<nrparts; i++) {
		parts[i].size = fl->img->size;
		parts[i].offset = fl->img->pl_flash_base;
		parts[i].name = names;

		strcpy(names, fl->img->name);
#ifdef CONFIG_MTD_REDBOOT_PARTS_READONLY
		if (!memcmp(names, "RedBoot", 8) ||
				!memcmp(names, "RedBoot config", 15) ||
				!memcmp(names, "FIS directory", 14)) {
			parts[i].mask_flags = MTD_WRITEABLE;
		}
#endif
		names += strlen(names)+1;

#ifdef CONFIG_MTD_REDBOOT_PARTS_UNALLOCATED
		if(fl->next && fl->img->pl_flash_base + fl->img->size + master->erasesize <= fl->next->img->pl_flash_base) {
			i++;
			parts[i].offset = parts[i-1].size + parts[i-1].offset;
			parts[i].size = fl->next->img->pl_flash_base - parts[i].offset;
			parts[i].name = nullname;
		}
#endif
		tmp_fl = fl;
		fl = fl->next;
		kfree(tmp_fl);
	}
	ret = nrparts;
	*pparts = parts;
 out:
	while (fl) {
		struct fis_list *old = fl;
		fl = fl->next;
		kfree(old);
	}
	vfree(buf);
	return ret;
}

static struct mtd_part_parser redboot_parser = {
	.owner = THIS_MODULE,
	.parse_fn = parse_redboot_partitions,
	.name = "RedBoot",
};

static int __init redboot_parser_init(void)
{
	return register_mtd_parser(&redboot_parser);
}

static void __exit redboot_parser_exit(void)
{
	deregister_mtd_parser(&redboot_parser);
}

postcore_initcall_sync(redboot_parser_init);
module_exit(redboot_parser_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Red Hat, Inc. - David Woodhouse <dwmw2@cambridge.redhat.com>");
MODULE_DESCRIPTION("Parsing code for RedBoot Flash Image System (FIS) tables");
