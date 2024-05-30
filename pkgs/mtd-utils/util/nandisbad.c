/*
 *  This is a tool that checks if a block is bad.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: nandisbad.c
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

#define PROGRAM "nandisbad"
#define VERSION "$Revision: 1.00 $"

#define MAX_PAGE_SIZE	2048
#define MAX_OOB_SIZE	64


void display_help (void)
{
	printf("Usage: nandisbad [OPTION] MTD_DEVICE\n"
	       "Return failure and message if block is bad, return success and no message if block is good.Return success and error message if system call has failed\n"
	       "\n"
	       "  -f [ARG],  --from [ARG]   Block address \n"
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
int opt_from=0;

void process_options (int argc, char *argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "f:";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
		   	{"from",  required_argument, 0, 'f'},
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
		case 'f':
			opt_from = strtoll (optarg,NULL,0);
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
	int fd;
	struct mtd_info_user meminfo;
	loff_t offs;
	int ret;

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

	offs = opt_from;

	if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
		perror("ioctl(MEMGETBADBLOCK)");
		close(fd);
		return 0;
	}
	
	if (ret == 1) {
		printf("Bad block detected at 0x%08x\n", (int) offs);
		close(fd);
		return -1;
	}
	else
	{
		//block is valid
		close(fd);
		return 0;
	}

}
