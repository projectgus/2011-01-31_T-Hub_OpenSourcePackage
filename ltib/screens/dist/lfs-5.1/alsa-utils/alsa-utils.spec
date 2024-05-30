%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Utilities for ALSA (Advanced Linux Sound Architecture)
Name            : alsa-utils
Version         : 1.0.10
Release         : 0
License         : GPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : Applications/System
Source          : alsa-utils-%{version}.tar.bz2
Patch1          : alsa-utils-1.0.10-cf.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch1 -p1

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --disable-nls
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
