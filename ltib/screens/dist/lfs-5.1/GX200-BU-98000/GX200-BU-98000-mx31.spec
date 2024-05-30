%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : MBX OpenGL ES (GX200)
Name            : GX200-BU-98000
Version         : r2p0.00bet0
Release         : 0
License         : Proprietary
Vendor          : ARM Ltd & Imagination Technologies Ltd
Packager        : Quinn Jensen
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}-%{version}
Prefix          : %{pfx}
%Description
This package provides proprietary binary kernel modules, libraries, and
test code built from the MBX OpenGL ES (GX200) DDK. The Linux OpenGL ES
DDK implements OpenGL-ES for MBX and variants along with 2D acceleration
via MBX within the Linux operating system. The target kernel version for
the DDK is Linux 2.6. The OpenGL-ES driver is ported from the reference
OpenGL-ES for MBX and variants. Hardware 2D acceleration is achieved
implementing the freedesktop.org KDrive X server in terms of the PVR2D
loadable module for MBX.

%{summary}

%Prep
%setup

%Build

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}
cp -a etc lib usr $RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
