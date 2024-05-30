***********  T-Hub open source delivery *********************************************************

Copyright (C) 2006 - 2011 Sagemcom All rights reserved
This file describes the delivery of the open source distribution used on T-Hub product

*************************************************************************************************

This folder contains:

GNU_GPL_LGPL.pdf	PDF document including GPL/LGPL licensing information.

ltib                    Folder including:
                        * imx31_ads_20070511-rel4-ltib.zip
                             Freescale BSP including source packages, build environment (ltib) 
                             and documentation.
                        * thub-ltib-imx31ads-20070511-1.0.tar.gz
                             Sagemcom modified version of ltib build environment.

pkgs                    Folder including the distributable additionnal or/and modified 
                        open source packages used by Sagemcom to build T-Hub software. 
                        The particular files and their corresponding licenses are indicated below:

Packages                 				License		Modified (Y/N)
------------------------------- 			---------	--------------

alsa-lib-1.0.11rc2.p5.tar.gz				LGPL			Y
atk-1.6.1.tar.bz2					LGPL			N
cairo-1.6.4.tar.gz					MPL			N
clutter-0.8.0_1.7.tar.gz				LGPL			Y
ffmpeg-0.4.9.20071114T0500_2.2.tar.gz			LGPL			Y
fontconfig-2.4.2.tar.gz					MIT			N
glib-2.14.6.tar.gz					LGPL			N
gstffmpeg-0.10.7_1.7.tar.gz				LGPL			Y
gst-plugins-bad-0.10.7_1.7.tar.gz			LGPL			Y
gst-plugins-base-0.10.22_1.3.tar.gz			LGPL			Y
gst-plugins-good-0.10.14_1.2.tar.gz			LGPL			Y
gst-plugins-ugly-0.10.6_1.5.tar.gz			LGPL			Y
gstreamer-0.10.22_1.3.tar.gz				LGPL			Y
gtk2-2.4.9.p1.tar.gz					LGPL			Y
iptables-1.3.8.tar.bz2					GPL			N
iputils-s20071127.tar.bz2				GPL			N
kernel-2.6.19-screens-sagem-pub-5.41.p13.tar.gz 	GPL			Y
libmms-0.4.5.tar.gz					LGPL			Y
liboil-0.3.16.tar.gz					Two clause BSD		N
module-init-tools-3.1-pre5.tar.bz2			GPL			N
mtd-utils-20060302.p3.tar.gz				GPL			Y
pango-1.20.2.tar.gz					LGPL			N
pixman-0.10.0.tar.gz					MPL			N
tiff-3.8.2.tar.gz					Open source		N
tslib-1.7.1.tar.gz					GPL			Y
udev-091_1.1.tar.gz					GPL			Y
wireless-tools-29.tar.gz				GPL			N


In order to compile the Sagemcom T-Hub distribution, you need to follow the steps below:

1. unzip Freescale original BSP under ltib directory: imx31_ads_20070511-rel4-ltib.zip.

	unzip imx31_ads_20070511-rel4-ltib.zip


2. Inside folder imx31_ads_20070511-rel4-ltib, run ./install and follow the instructions. 
During this stage you will install on your PC, under /opt/freescale, the toolchain and 
PC tools to build the software. You will need the root rights to perform some of the tasks.
Please note that installation has NOT been tested on all PC distributions, you might encounter 
some problems during the installation. The installation has been tested successfully 
on Fedora core 4,5,6 and Ubuntu/8.04.

	cd imx31_ads_20070511-rel4-ltib
	./install

	<follow Freescale install tool instructions >

3. Under ltib/imx31_ads_20070511-rel4-ltib/pkgs you will find all the open source tarballs 
provided inside Freescale BSP. Please refer to freescale imx31_ads_20070511-rel4-ltib/EULA 
to see the list of the packages.

4. Under ltib directoy, uncompress Sagemcom T-Hub package thub-ltib-imx31ads-20070511-1.0.tar.gz

	tar xzf thub-ltib-imx31ads-20070511-1.0.tar.gz

5. Under ltib/screens/pkgs copy all the packages provided under pkgs 
and ltib/imx31_ads_20070511-rel4-ltib/pkgs. Alternatively you can create a unique package pool 
in the place of your choice on your PC and modify .ltibrc file to define the path 
to your personnal package pool.

6. Run ./ltib

NOTE 1: clutter sources are NOT compiled. You will need to install an Open GLES library 
and its includes in order to fullfill the compilation of this package.

NOTE 2: If ltib is complaining about missing tarball make sure that you have copied 
all the open source code under pkgs from either Freescale package pool or Sagemcom package pool.
