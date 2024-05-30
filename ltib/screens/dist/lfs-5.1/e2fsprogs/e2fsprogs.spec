%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Second extended (ext2) filesystem and ext3 utilities
Name            : e2fsprogs
Version         : 1.34
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Base
Source          : e2fsprogs-1.34.tar.gz
Patch0          : e2fsprogs-1.34-msgmerge.patch
Patch1          : e2fsprogs-1.34-add-scsi-disk-major.patch
Patch2          : e2fsprogs-1.34-cf-bitops_h.patch
Patch3          : e2fsprogs-1.34-fix-lib-links.patch
Patch4          : e2fsprogs-1.34-make-3.81.patch
Patch5          : e2fsprogs-1.34-powerpc64.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1

%Build
# this is needed for uclibc or you get:
# cp ./libgnuintl.h libintl.h
# cp: cannot create regular file `libintl.h': Permission denied
chmod +w intl/libintl.h
CC="gcc" BUILD_CC="$BUILDCC" \
./configure --enable-dynamic-e2fsck --enable-elf-shlibs --prefix=%{_prefix} --host=$CFGHOST --mandir=%{_mandir} --disable-nls
make -j1 libs progs docs

%Install
rm -rf $RPM_BUILD_ROOT
make -j1 DESTDIR=$RPM_BUILD_ROOT/%{pfx} install install-libs
#strip 
STRIP=${TOOLCHAIN_PREFIX}strip
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/chattr
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/lsattr
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/uuidgen
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libblkid.so.1.0
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libcom_err.so.2.1
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libe2p.so.2.3
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libext2fs.so.2.4
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libss.so.2.0
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libuuid.so.1.2
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/sbin/*

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


