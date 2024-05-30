%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A Linux filesystem designed to be simple, small, and to compress things well.
Name            : cramfs
Version         : 1.1
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Olivia Yin
Group           : Applications/System
Source          : %{name}-%{version}.tar.gz
Patch0          : cramfsck-swap-endian.patch
Patch1          : mkcramfs-swap-endian.patch
Patch2          : cramfs-devtable.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1
%patch1 -p1
%patch2 -p1

%Build
make -f GNUmakefile 

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin
install -m 755 mkcramfs $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin/mkfs.cramfs
install -m 755 cramfsck $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin/fsck.cramfs

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
