%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Utilities for ALSA (Advanced Linux Sound Architecture)
Name            : alsa-utils
Version         : 1.0.6
# 1.0.6 is compatible with linux 2.6.10 kernels
#   (see linux/include/sound/version.h)
Release         : 0
License         : GPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : Applications/System
Source          : alsa-utils-%{version}.tar.bz2
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
