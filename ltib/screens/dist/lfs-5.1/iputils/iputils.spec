%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Utilities for converting IP addresses and masks between various formats.
Name            : iputils
Version         : s20071127
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Olivia Yin
Group           : Applications/System
Source          : %{name}-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup

%Build
make CC="gcc"

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/bin
make
install -s -m 755 ping $RPM_BUILD_ROOT/%{pfx}/bin/ping


%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
