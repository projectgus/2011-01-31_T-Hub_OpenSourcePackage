%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Memory Technology Device tools
Name            : mtd-utils
Version         : 20060302.p3
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/System
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}


%Prep
%setup -n mtd-utils

%Build
make -C util

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin
make -C util DESTDIR=$RPM_BUILD_ROOT/%{pfx} SBINDIR=%{_prefix}/bin MANDIR=%{_mandir} install
#strip 
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/opt/freescale/ltib/usr/bin/*

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
