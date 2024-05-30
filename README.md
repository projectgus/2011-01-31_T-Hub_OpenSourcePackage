# Telstra T-Hub Open Source archive

This is the contents of an archive of source code that I was sent in 2011 on a burned CD-R, in response to a blog post:
https://www.projectgus.com/2010/11/telstra-violating-gpl/

A historical curio now, but someone recently asked me if I still had a copy of it.

The original CD contained a two-tier set of tar archives of source, that I've extracted in order to be suitable for sharing on GitHub. The original structure was:

```
❯ sha256sum 2011-01-31_T-Hub_OpenSourcePackage.tar
72298369501a5a51fb217c9088ff25cf2c2e117f4b869dbfee7cb20d7781c693  2011-01-31_T-Hub_OpenSourcePackage.tar

❯ tar tvf 2011-01-31_T-Hub_OpenSourcePackage.tar
drwxrwxrwx 0/0               0 2011-01-29 04:26 2011-01-31_T-Hub_OpenSourcePackage/
-rwxrwxrwx 0/0           51820 2011-01-26 02:17 2011-01-31_T-Hub_OpenSourcePackage/GNU_GPL_LGPL.pdf
drwxrwxrwx 0/0               0 2011-01-29 04:15 2011-01-31_T-Hub_OpenSourcePackage/ltib/
-rwxrwxrwx 0/0       498263414 2011-01-24 21:28 2011-01-31_T-Hub_OpenSourcePackage/ltib/imx31_ads_20070511-rel4-ltib.zip
-rwxrwxrwx 0/0              67 2011-01-24 21:49 2011-01-31_T-Hub_OpenSourcePackage/ltib/imx31_ads_20070511-rel4-ltib.zip.md5
-rwxrwxrwx 0/0         5055212 2011-01-29 04:14 2011-01-31_T-Hub_OpenSourcePackage/ltib/thub-ltib-imx31ads-20070511-1.0.tar.gz
-rwxrwxrwx 0/0              73 2011-01-29 04:14 2011-01-31_T-Hub_OpenSourcePackage/ltib/thub-ltib-imx31ads-20070511-1.0.tar.gz.md5
drwxrwxrwx 0/0               0 2011-01-29 03:07 2011-01-31_T-Hub_OpenSourcePackage/pkgs/
-rwxrwxrwx 0/0          979403 2011-01-24 21:56 2011-01-31_T-Hub_OpenSourcePackage/pkgs/alsa-lib-1.0.11rc2.p5.tar.gz
-rwxrwxrwx 0/0              63 2011-01-24 21:56 2011-01-31_T-Hub_OpenSourcePackage/pkgs/alsa-lib-1.0.11rc2.p5.tar.gz.md5
-rwxrwxrwx 0/0          493415 2011-01-24 22:02 2011-01-31_T-Hub_OpenSourcePackage/pkgs/atk-1.6.1.tar.bz2
-rwxrwxrwx 0/0              52 2011-01-24 22:02 2011-01-31_T-Hub_OpenSourcePackage/pkgs/atk-1.6.1.tar.bz2.md5
-rwxrwxrwx 0/0        14981120 2011-01-29 02:19 2011-01-31_T-Hub_OpenSourcePackage/pkgs/cairo-1.6.4.tar.gz
-rwxrwxrwx 0/0              53 2011-01-29 02:17 2011-01-31_T-Hub_OpenSourcePackage/pkgs/cairo-1.6.4.tar.gz.md5
-rwxrwxrwx 0/0         2055729 2011-01-24 22:12 2011-01-31_T-Hub_OpenSourcePackage/pkgs/clutter-0.8.0_1.7.tar.gz
-rwxrwxrwx 0/0              59 2011-01-24 22:12 2011-01-31_T-Hub_OpenSourcePackage/pkgs/clutter-0.8.0_1.7.tar.gz.md5
-rwxrwxrwx 0/0         2885527 2011-01-24 22:15 2011-01-31_T-Hub_OpenSourcePackage/pkgs/ffmpeg-0.4.9.20071114T0500_2.2.tar.gz
-rwxrwxrwx 0/0              72 2011-01-24 22:15 2011-01-31_T-Hub_OpenSourcePackage/pkgs/ffmpeg-0.4.9.20071114T0500_2.2.tar.gz.md5
-rwxrwxrwx 0/0         1278340 2011-01-29 02:20 2011-01-31_T-Hub_OpenSourcePackage/pkgs/fontconfig-2.4.2.tar.gz
-rwxrwxrwx 0/0              58 2011-01-29 02:20 2011-01-31_T-Hub_OpenSourcePackage/pkgs/fontconfig-2.4.2.tar.gz.md5
-rwxrwxrwx 0/0         4685643 2011-01-24 22:17 2011-01-31_T-Hub_OpenSourcePackage/pkgs/glib-2.14.6.tar.gz
-rwxrwxrwx 0/0              53 2011-01-24 22:17 2011-01-31_T-Hub_OpenSourcePackage/pkgs/glib-2.14.6.tar.gz.md5
-rwxrwxrwx 0/0         3785706 2011-01-24 22:22 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-bad-0.10.7_1.7.tar.gz
-rwxrwxrwx 0/0              68 2011-01-24 22:22 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-bad-0.10.7_1.7.tar.gz.md5
-rwxrwxrwx 0/0         3008712 2011-01-24 22:23 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-base-0.10.22_1.3.tar.gz
-rwxrwxrwx 0/0              70 2011-01-24 22:23 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-base-0.10.22_1.3.tar.gz.md5
-rwxrwxrwx 0/0         3304840 2011-01-24 22:24 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-good-0.10.14_1.2.tar.gz
-rwxrwxrwx 0/0              70 2011-01-24 22:24 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-good-0.10.14_1.2.tar.gz.md5
-rwxrwxrwx 0/0         1112172 2011-01-24 22:26 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-ugly-0.10.6_1.5.tar.gz
-rwxrwxrwx 0/0              69 2011-01-24 22:26 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gst-plugins-ugly-0.10.6_1.5.tar.gz.md5
-rwxrwxrwx 0/0          594647 2011-01-24 22:21 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gstffmpeg-0.10.7_1.7.tar.gz
-rwxrwxrwx 0/0              62 2011-01-24 22:21 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gstffmpeg-0.10.7_1.7.tar.gz.md5
-rwxrwxrwx 0/0         3680598 2011-01-24 22:29 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gstreamer-0.10.22_1.3.tar.gz
-rwxrwxrwx 0/0              63 2011-01-24 22:29 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gstreamer-0.10.22_1.3.tar.gz.md5
-rwxrwxrwx 0/0        14596540 2011-01-27 22:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gtk2-2.4.9.p1.tar.gz
-rwxrwxrwx 0/0              55 2011-01-27 22:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/gtk2-2.4.9.p1.tar.gz.md5
-rwxrwxrwx 0/0          172584 2011-01-25 00:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/iptables-1.3.8.tar.bz2
-rwxrwxrwx 0/0              57 2011-01-25 00:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/iptables-1.3.8.tar.bz2.md5
-rwxrwxrwx 0/0           88796 2011-01-25 00:29 2011-01-31_T-Hub_OpenSourcePackage/pkgs/iputils-s20071127.tar.bz2
-rwxrwxrwx 0/0              60 2011-01-25 00:29 2011-01-31_T-Hub_OpenSourcePackage/pkgs/iputils-s20071127.tar.bz2.md5
-rwxrwxrwx 0/0        57896628 2011-01-25 00:37 2011-01-31_T-Hub_OpenSourcePackage/pkgs/kernel-2.6.19-screens-sagem-pub-5.41.p13.tar.gz
-rwxrwxrwx 0/0              82 2011-01-25 00:37 2011-01-31_T-Hub_OpenSourcePackage/pkgs/kernel-2.6.19-screens-sagem-pub-5.41.p13.tar.gz.md5
-rwxrwxrwx 0/0          351074 2011-01-25 00:40 2011-01-31_T-Hub_OpenSourcePackage/pkgs/libmms-0.4.5.tar.gz
-rwxrwxrwx 0/0              54 2011-01-25 00:40 2011-01-31_T-Hub_OpenSourcePackage/pkgs/libmms-0.4.5.tar.gz.md5
-rwxrwxrwx 0/0          855765 2011-01-29 02:20 2011-01-31_T-Hub_OpenSourcePackage/pkgs/liboil-0.3.16.tar.gz
-rwxrwxrwx 0/0              54 2011-01-29 02:20 2011-01-31_T-Hub_OpenSourcePackage/pkgs/liboil-0.3.16.tar.gz.md5
-rwxrwxrwx 0/0          122808 2011-01-29 02:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/module-init-tools-3.1-pre5.tar.bz2
-rwxrwxrwx 0/0              69 2011-01-29 02:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/module-init-tools-3.1-pre5.tar.bz2.md5
-rwxrwxrwx 0/0             248 2011-01-29 02:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/module-init-tools-3.1-pre5.tar.bz2.sign
-rwxrwxrwx 0/0              73 2011-01-29 02:28 2011-01-31_T-Hub_OpenSourcePackage/pkgs/module-init-tools-3.1-pre5.tar.bz2.sign.md5
-rwxrwxrwx 0/0          464498 2011-01-25 00:42 2011-01-31_T-Hub_OpenSourcePackage/pkgs/mtd-utils-20060302.p3.tar.gz
-rwxrwxrwx 0/0              63 2011-01-25 00:42 2011-01-31_T-Hub_OpenSourcePackage/pkgs/mtd-utils-20060302.p3.tar.gz.md5
-rwxrwxrwx 0/0         2066620 2011-01-25 00:43 2011-01-31_T-Hub_OpenSourcePackage/pkgs/pango-1.20.2.tar.gz
-rwxrwxrwx 0/0              54 2011-01-25 00:43 2011-01-31_T-Hub_OpenSourcePackage/pkgs/pango-1.20.2.tar.gz.md5
-rwxrwxrwx 0/0         1894400 2011-01-29 02:22 2011-01-31_T-Hub_OpenSourcePackage/pkgs/pixman-0.10.0.tar.gz
-rwxrwxrwx 0/0              55 2011-01-29 02:22 2011-01-31_T-Hub_OpenSourcePackage/pkgs/pixman-0.10.0.tar.gz.md5
-rwxrwxrwx 0/0         1336295 2011-01-29 02:23 2011-01-31_T-Hub_OpenSourcePackage/pkgs/tiff-3.8.2.tar.gz
-rwxrwxrwx 0/0              52 2011-01-29 02:23 2011-01-31_T-Hub_OpenSourcePackage/pkgs/tiff-3.8.2.tar.gz.md5
-rwxrwxrwx 0/0           62536 2011-01-29 02:25 2011-01-31_T-Hub_OpenSourcePackage/pkgs/tslib-1.7.1.tar.gz
-rwxrwxrwx 0/0              53 2011-01-29 02:25 2011-01-31_T-Hub_OpenSourcePackage/pkgs/tslib-1.7.1.tar.gz.md5
-rwxrwxrwx 0/0          322667 2011-01-29 02:25 2011-01-31_T-Hub_OpenSourcePackage/pkgs/udev-091_1.1.tar.gz
-rwxrwxrwx 0/0              54 2011-01-29 02:25 2011-01-31_T-Hub_OpenSourcePackage/pkgs/udev-091_1.1.tar.gz.md5
-rwxrwxrwx 0/0          299205 2011-01-28 02:17 2011-01-31_T-Hub_OpenSourcePackage/pkgs/wireless-tools-29.tar.gz
-rwxrwxrwx 0/0              59 2011-01-28 02:17 2011-01-31_T-Hub_OpenSourcePackage/pkgs/wireless-tools-29.tar.gz.md5
-rwxrwxrwx 0/0            4175 2011-01-29 04:25 2011-01-31_T-Hub_OpenSourcePackage/README.TXT
```

(All the .md5 hashes matched, pretty decent for a burned CD that's over 13 years old.)

See the original `README.TXT` in this directory for the Sagemcom (OEM) build instructions.

Needless to say this was provided to me with absolutely no warranty, and is now provided to you under the same terms. :)
