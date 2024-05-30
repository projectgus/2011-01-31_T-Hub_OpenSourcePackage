%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A library of optimized inner loops
Name            : liboil
Version         : 0.3.16
Release         : 1
License         : Two-Clause BSD
Vendor          : Freescale
Packager        : Kurt Mahan
Group           : System Environment/Libraries
Source          : liboil-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup

%Build
./configure NM=nm --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make -j1

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
