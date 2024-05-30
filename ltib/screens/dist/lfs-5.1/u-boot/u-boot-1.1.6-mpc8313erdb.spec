%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Universal Bootloader firmware
Name            : u-boot
Version         : 1.1.6
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Vivienne Li
Group           : Applications/System
Source          : %{name}-%{version}-mpc83xx.tgz
Patch0          : u-boot-1.1.6-mpc8313erdb-general.patch
Patch1          : u-boot-1.1.6-mpc8313erdb-cpu.patch
Patch2          : u-boot-1.1.6-mpc8313erdb-board.patch
Patch3          : u-boot-1.1.6-mpc8313erdb-vsc7385.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
 U-Boot 1.1.6 configured for MPC8313E RDB board.
 The patch file used is based on source changes from
 http://opensource.freescale.com/pub/scm/u-boot-83xx.git (a public git repository)
 using the tree with sha1 value 21fb83dfa2dcba7982eeb54f4ca7bc1056582c1e.

%{summary}

All source and patches from Freescale.

%Prep
%setup -n %{name}-%{version}
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1

%Build
PKG_U_BOOT_CONFIG_TYPE=${PKG_U_BOOT_CONFIG_TYPE:-MPC8313ERDB_config}
make HOSTCC="$BUILDCC" CROSS_COMPILE=$TOOLCHAIN_PREFIX $PKG_U_BOOT_CONFIG_TYPE
make HOSTCC="$BUILDCC" HOSTSTRIP="$BUILDSTRIP" \
     CROSS_COMPILE=$TOOLCHAIN_PREFIX $PKG_U_BOOT_BUILD_ARGS all

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/boot
for i in u-boot.bin u-boot examples/vsc7385_load/vsc7385_load.bin
do
    cp $i $RPM_BUILD_ROOT/%{pfx}/boot
done

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
