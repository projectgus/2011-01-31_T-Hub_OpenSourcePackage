%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A screen manager that supports multiple logins on one terminal
Name            : screen
Version         : 4.0.2
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/System
URL             : ftp://ftp.gnu.org/pub/pub/gnu/screen/
Source          : %{name}-%{version}.tar.gz
Patch1          : screen-4.0.2-cross.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch1 -p1 

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
