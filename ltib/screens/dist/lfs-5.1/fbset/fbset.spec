%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Utility for configuring framebuffer devices
Name            : fbset
Version         : 2.1
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : WMSG
Group           : Graphics
Source          : %{name}-%{version}.tar.gz
Patch1          : fbset-2.1-ltib_makefile_fixup.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch1 -p1 

%Build
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
