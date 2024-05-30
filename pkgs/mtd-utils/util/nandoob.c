/*
 *  This is a tool to manage Out Of Band (OOB) part of the flash.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: nandoob.c
 *  Creation date: 28/04/2010
 *  Author: David Giguet, Sagemcom
 *
 *  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  Write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA to
 *  receive a copy of the GNU General Public License.
 *  This Copyright notice should not be removed
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"

#define PROGRAM "nandoob"
#define VERSION "$Revision: 1.00 $"

#define MAX_PAGE_SIZE	2048
#define MAX_OOB_SIZE	64
unsigned char ag_jffs2_marker[8]={0x85,0x19,0x03,0x20,0x8,0x0,0x0,0x0};

/*
 * Buffer array used for writing data
 */

unsigned char oobbuf[MAX_OOB_SIZE];

void display_help (void)
{
	printf("Usage: nandstat [OPTION] MTD_DEVICE\n"
	       "Output statistics of MTD device.\n"
	       "\n"
	       "  -i,        --info         Display flash information and statistics  \n"
	       "  -r,        --read         Display Out Of Band (OOB) information \n"
	       "  -f [ARG],  --from [ARG]   Display OOB from address ARG \n"
	       "  -l [ARG],  --length [ARG] Display ARG bytes of OOB data \n"
	       "  -b,        --badblocks    Disblay the number and placement of bad blocks \n"
	       );
	exit(0);
}

void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
	       "\n"
	       "Copyright (C) 2010 Sagemcom David GIGUET \n"
	       "\n");
	exit(0);
}

char 	*mtd_device;
int info=0;
int readoob=0;
int opt_from=0;
int opt_length=-1;
int opt_badblocks=0;

void process_options (int argc, char *argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "irf:l:b";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"info", no_argument, 0, 'i'},
		   	{"read",  no_argument, 0, 'r'},
		   	{"from",  required_argument, 0, 'f'},
		   	{"length",  required_argument, 0, 'l'},
		   	{"badblocks",  no_argument, 0, 'b'},
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				    long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				display_help();
				break;
			case 1:
				display_version();
				break;
			}
			break;
		case 'i':
			info = 1;
			break;
		case 'r':
			readoob = 1;
			break;
		case 'f':
			opt_from = strtoll (optarg,NULL,0);
			break;
		case 'l':
			opt_length = strtoll (optarg,NULL,0);
			break;
		case 'b':
			opt_badblocks = 1;
			break;
		case '?':
			error = 1;
			break;
		}
	}

	if ((argc - optind) != 1 || error)
		display_help ();

	mtd_device = argv[optind];
}

/*
 * Main program
 */
int main(int argc, char **argv)
{
	int fd, pagelen, baderaseblock, blockstart = -1;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	loff_t offs;
	int ret, i, j;
	struct nand_oobinfo current_oobinfo;

	process_options(argc, argv);

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		perror("open flash");
		exit(1);
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}

	/* Read the current oob info */
	if (ioctl (fd, MEMGETOOBSEL, &current_oobinfo) != 0) {
		perror ("MEMGETOOBSEL");
		close (fd);
		exit (1);
	}

	if (opt_length==-1) opt_length=meminfo.size;

	if (opt_from<0)
	{
		printf("Invalid parameter: negative value (%d)\n",opt_from);
		exit(1);
	}
	else if (opt_from>meminfo.size)
	{
		printf("Invalid parameter from (0x%x), it cannot be greater than partition size (0x%x)\n",opt_from,meminfo.size);
		exit(1);
	}
	else
	{
		opt_from=(opt_from/meminfo.erasesize)*meminfo.erasesize;
	}

	blockstart=opt_from;

	if (opt_length<=0)
	{
		opt_length=meminfo.erasesize;
	}
	else if (opt_length>meminfo.size-opt_from)
	{
		opt_length=meminfo.size-opt_from;
	}

	if (info)
	{
	   printf("Flash Informations:\n");
	   printf("type=0x%x\n",meminfo.type);
	   printf("flags=0x%x\n",meminfo.flags);
	   printf("Total size=%d bytes\n",meminfo.size);
	   printf("Block size=%d bytes\n",meminfo.erasesize);
	   printf("Page size=%d bytes\n",meminfo.oobblock);
	   printf("Amount of OOB data per block=%d bytes\n",meminfo.oobsize);
	   printf("ECC type=0x%x\n",meminfo.ecctype);
	   printf("ECC size=%d bytes\n\n",meminfo.eccsize);
	   printf("OOB informations:\n");
	   for (i=0;i<8;i++)
	   {
	      for (j=0;j<2;j++)
	      {
	         printf("oobfree[%d][%d]=%d\n",i,j,current_oobinfo.oobfree[i][j]);
	      }
	   }
	   for (i=0;i<32;i++)
	   {
		printf("eccpos[%d]=%d\n",i,current_oobinfo.eccpos[i]);
	   }
	}

	oob.length = meminfo.oobsize;
	oob.ptr = oobbuf;
	offs = blockstart;

	pagelen = meminfo.oobblock;
	baderaseblock = 0;

	offs = 0;

	if (opt_badblocks)
	{
		int k,l;
		int dirtyoob=0;

		do {

			int current_block_is_bad=0;
			int current_block_is_dirty=0;
			/* Check all the blocks in an erase block for bad blocks */
			if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
				perror("ioctl(MEMGETBADBLOCK)");
			}

			if (ret==1)
			{
				printf("Bad block at 0x%08x\n",(unsigned int)offs);
				baderaseblock++;
				current_block_is_bad=1;
			}

			for (k=0;k<meminfo.erasesize;k+=meminfo.oobblock)
			{
				oob.start=offs+k;
				oob.ptr=oobbuf;

				if ((ret = ioctl(fd, MEMREADOOB, &oob)) < 0) {
					perror("ioctl(MEMREADOOB");
				}

				for (l=0;l<oob.length;l++)
				{
					if (oobbuf[l]!=0xff)
					{
						if ((l>=current_oobinfo.oobfree[0][0])&&(l<current_oobinfo.oobfree[0][0]+current_oobinfo.oobfree[0][1]))
						{
							//this could be a jffs2 clean marker
							if (oobbuf[l]==ag_jffs2_marker[l-current_oobinfo.oobfree[0][0]])
							{
								continue;
							}
						}
						if ((oobbuf[l]==0x0)&&(current_block_is_bad))
						{
							continue;
						}
						current_block_is_dirty=1;
						break;
					}
				}
				if (current_block_is_dirty)
				{
					dirtyoob++;
					printf("dirty oob at at 0x%08x: ",(unsigned int)offs);
					for (l=0;l<oob.length;l++)
					{
						printf("%02x ",oobbuf[l]);
					}
					printf("\n");
					break;
				}
			}

			offs += meminfo.erasesize;
		} while (offs < meminfo.size);

		printf("Number of bad blocks=%d, Number of dirty obb=%d\n",baderaseblock,dirtyoob);
	}

	offs = blockstart;

	if (readoob)
	{
		do {

			printf ("Reading block 0x%08x ------------------------------------------------------\n\n", (unsigned int)offs);

			/* Check all the blocks in an erase block for bad blocks */
			if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
				perror("ioctl(MEMGETBADBLOCK)");
				goto closeall;
			}
			if (ret == 1) {
				baderaseblock++;
				printf ("Bad block at 0%08x\n", (int) offs);
			}
			for (i=0;i<meminfo.erasesize;i+=pagelen)
			{
				oob.start=offs+i;
				oob.ptr=oobbuf;

				if ((ret = ioctl(fd, MEMREADOOB, &oob)) < 0) {
					perror("ioctl(MEMREADOOB)");
					goto closeall;
				}

				printf("Reading page 0x%08x, oob block:\n",(unsigned int)(offs+i));
				for (j=0;j<oob.length;j++)
				{
					printf("%02x ",oobbuf[j]);
				}
				printf("\n");
			}

			offs += meminfo.erasesize;
			opt_length -= meminfo.erasesize;
		} while ((offs < meminfo.size)&&(opt_length>0));
	}


 closeall: 
	close(fd);


	/* Return happy */
	return 0;
}
