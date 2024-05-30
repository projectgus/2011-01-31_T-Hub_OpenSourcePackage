%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Point-to-Point Protocol daemon
Name            : ppp
Version         : 2.4.1
Release         : 1
License         : BSD
Vendor          : Freescale
Packager        : Michael Barker
Group           : System Environment/Daemons
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --mandir=%{_mandir}
make -j1

%Install
rm -rf $RPM_BUILD_ROOT
make -j1 install INSTALL=install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
mkdir -p $RPM_BUILD_ROOT/%{pfx}/var/lock

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
