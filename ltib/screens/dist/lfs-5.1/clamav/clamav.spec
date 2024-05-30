%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Clam AntiVirus is a GPL anti-virus toolkit for UNIX
Name            : clamav
Version         : 0.88
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Sam Siu
Group           : System Environment/Daemons
Source          : %{name}-%{version}.tar.gz
Patch1          : clamav-0.88-configure-cross.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch1 -p1

%Build
ac_cv_func_snprintf=no ac_cv_lib_z_inflateEnd=yes ac_cv_func_setpgrp_void=yes ac_cv_have_accrights_in_msghdr=yes \
ac_cv_have_control_in_msghdr=yes ac_cv_have_accrights_in_msghdr=no \
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --disable-clamav --disable-zlib-vcheck \
--disable-bzip2 --disable-dsig
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
