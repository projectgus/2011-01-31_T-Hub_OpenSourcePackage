%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A libraries for ALSA (Advanced Linux Sound Architecture)
Name            : alsa-lib
Version         : 1.0.6
# 1.0.6 is compatible with linux 2.6.10 kernels
#   (see linux/include/sound/version.h)
Release         : 0
License         : LGPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : System Environment/Libraries
Source          : alsa-lib-%{version}.tar.bz2
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
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/alsa-lib/*/*.la

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
