%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : a free library for arbitrary precision arithmetic, operating on signed integers, rational numbers, and floating point numbers
Name            : gmp
Version         : 4.1.4
Release         : 1
License         : LGPL
Vendor          : Freescale
Packager        : Kim Phillips
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
