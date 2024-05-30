%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Universal Bootloader firmware
Name            : u-boot
Version         : 1.1.2
Release         : pq3_20050113_0
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/System
Source          : u-boot-1.1.1.tar.bz2
Patch0          : u-boot-1.1.1-MPC8555CDS_K26_20050228-3.0.patch.gz
Patch1          : u-boot-fsl-1.1.2-General.patch
Patch2          : u-boot-fsl-1.1.2-MPC8540EVAL.patch
Patch3          : u-boot-fsl-1.1.2-MPC8555CDS_PCI.patch
Patch4          : u-boot-fsl-1.1.2-MPC8560ADS_FCC.patch
Patch5          : u-boot-fsl-1.1.2-PC8560ADS_debug_port.patch
Patch6          : u-boot-fsl-1.1.2-ct-8555.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n u-boot-1.1.1
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1

%Build
PKG_U_BOOT_CONFIG_TYPE=${PKG_U_BOOT_CONFIG_TYPE:-TQM823L_config}
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
