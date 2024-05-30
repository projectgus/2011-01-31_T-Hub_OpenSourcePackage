%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Universal Bootloader firmware
Name            : u-boot
Version         : 1.1.5
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : John Rigby
Group           : Applications/System
Source          : u-boot-1.1.5.tar.bz2
Patch1		: u-boot-1.1.5-mpc5200-0001-01Nov2006-Fix-make-3.81.patch
Patch2		: u-boot-1.1.5-mpc5200-0002-01Nov2006-Early-DRAM-and-MMU-init.patch
Patch3		: u-boot-1.1.5-mpc5200-0003-01Nov2006-Read-Ethernet-ADDR-from-EEPROM.patch
Patch4		: u-boot-1.1.5-mpc5200-0004-01Nov2006-Conditionally-skip-Net-PCI-and-IDE-init.patch
Patch5		: u-boot-1.1.5-mpc5200-0005-01Nov2006-Add-media5200-platform.patch
Patch6		: u-boot-1.1.5-mpc5200-0006-01Nov2006-Add-coralpa-framebuffer-driver.patch
Patch7		: u-boot-1.1.5-mpc5200-0007-01Nov2006-Add-FSL-Logo.patch
Patch8		: u-boot-1.1.5-mpc5200-0008-01Nov2006-Add-flash-write-status-reporting.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

This specfile attempts to recreate the u-boot binaries
preinstalled on the Lite5200B and Media5200 platforms.

%Prep
%setup -n u-boot-1.1.5
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1

%Build
export PATH=$UNSPOOF_PATH
PKG_U_BOOT_CONFIG_TYPE=${PKG_U_BOOT_CONFIG_TYPE:-lite5200b_config}
make distclean
make $PKG_U_BOOT_CONFIG_TYPE
make CROSS_COMPILE=powerpc-603e-linux-

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/boot
cp u-boot.bin $RPM_BUILD_ROOT/%{pfx}/boot
cp u-boot $RPM_BUILD_ROOT/%{pfx}/boot

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
