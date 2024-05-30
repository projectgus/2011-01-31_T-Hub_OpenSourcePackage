%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Openswan is an Open Source implementation of IPsec for the Linux operating system. Is it a code fork of the FreeS/WAN project, started by a few of the developers who were growing frustrated with the politics surrounding the FreeS/WAN project.
Name            : openswan
Version         : 2.3.0
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Kim Phillips
Group           : Applications/Internet
Source          : %{name}-%{version}.tar.gz
Patch0          : openswan-2.3-gcc4-fixes.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch0 -p1

%Build
make -j1 programs

%Install
rm -rf $RPM_BUILD_ROOT
make -j1 install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
