%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Netperf is a benchmark that can be used to measure the performance of many different types of networking
Name            : netperf
Version         : 2.4.0
Release         : 1
License         : Distributable
Vendor          : Freescale
Packager        : Kim Phillips
Group           : Development/Tools
Source          : %{name}-%{version}.tar.gz
Patch1          : netperf-2.4.0-mixed_endian_fp.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch1 -p1

%Build
ac_cv_func_setpgrp_void=yes ac_cv_func_sched_setaffinity=no \
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

