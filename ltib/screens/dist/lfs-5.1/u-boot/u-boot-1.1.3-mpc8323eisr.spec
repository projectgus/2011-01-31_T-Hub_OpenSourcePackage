%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Universal Bootloader firmware
Name            : u-boot
Version         : 1.1.3
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Michael Barker
Group           : Applications/System
Source          : %{name}-%{version}.tar.bz2
Patch0		: u-boot-1.1.3-mpc832x.patch
Patch1		: u-boot-1.1.3-mpc8323isr.patch
Patch2		: u-boot-1.1.3-mpc8323isr-pciclk-env.patch
Patch3		: u-boot-1.1.3-mpc8323isr-ltib.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

All source and patches from Freescale.

%Prep
%setup -n %{name}-%{version}
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1

%Build
PKG_U_BOOT_CONFIG_TYPE=${PKG_U_BOOT_CONFIG_TYPE:-MPC8323ISR_config}
make HOSTCC="$BUILDCC" CROSS_COMPILE=$TOOLCHAIN_PREFIX $PKG_U_BOOT_CONFIG_TYPE
make HOSTCC="$BUILDCC" HOSTSTRIP="$BUILDSTRIP" \
     CROSS_COMPILE=$TOOLCHAIN_PREFIX $PKG_U_BOOT_BUILD_ARGS all

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/boot
for i in u-boot.bin u-boot
do
    cp $i $RPM_BUILD_ROOT/%{pfx}/boot
done

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
