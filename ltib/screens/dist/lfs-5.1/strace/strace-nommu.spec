%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : trace system calls associated with a running process
Name            : strace
Version         : 4.4
Release         : 1
License         : BSD
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Debuggers
Source          : strace-ucd-20050906.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

Extracted from uClinux-dist-test-20050906.tar.bz2

%Prep
%setup -n strace

%Build
CFLAGS="-DHZ=100 -DHAVE_SYS_QUOTA_H" ac_cv_header_sys_reg_h=no \
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --mandir=%{_mandir}
make -f Makefile

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/share/man/man1
make -f Makefile install prefix=$RPM_BUILD_ROOT/%{pfx}/%{_prefix} mandir=$RPM_BUILD_ROOT/%{pfx}/%{_mandir}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
