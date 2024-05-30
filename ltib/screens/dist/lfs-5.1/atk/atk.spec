%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Interfaces for accessibility support
Name            : atk
Version         : 1.6.1
Release         : 1
License         : LGPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
./configure \
	--prefix=%{_prefix} --build=%{_build} --host=$CFGHOST \
	--enable-shared --disable-static
make

%Install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install

rm -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/include/*
%{pfx}/usr/lib/*
