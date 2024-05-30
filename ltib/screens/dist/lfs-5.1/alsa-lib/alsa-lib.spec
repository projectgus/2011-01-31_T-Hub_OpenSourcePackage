%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A libraries for ALSA (Advanced Linux Sound Architecture)
Name            : alsa-lib
Version         : 1.0.10
Release         : 0
License         : LGPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : System Environment/Libraries
Source          : alsa-lib-%{version}.tar.bz2
Patch0          : alsa-lib-uclibc-wordexp.patch
Patch1          : alsa-lib-1.0.10-cf.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1
%patch1 -p1

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/alsa-lib/smixer/*.la

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
