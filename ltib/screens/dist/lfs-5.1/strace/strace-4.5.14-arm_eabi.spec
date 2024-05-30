%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : trace system calls associated with a running process
Name            : strace
Version         : 4.5.14
Release         : 1
License         : BSD
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Debuggers
Source          : %{name}-%{version}.tar.bz2
Patch0          : strace-4.5.14-syscall_eabi.patch
Patch1          : strace-fix-arm-bad-syscall.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch0 -p1
%patch1 -p1

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --mandir=%{_mandir}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
