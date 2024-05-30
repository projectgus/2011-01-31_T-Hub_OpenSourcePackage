%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Apptrk - target executable for Codewarrior
Name            : AppTrk-m68k
Version         : 1.37.2
Release         : 0
License         : Freescale EULA
Vendor          : Freescale
Packager        : Matt Waddel
Group           : Development/Debuggers
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/MetroTRK
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup

%Build

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp AppTrk_mcf5329_rel $RPM_BUILD_ROOT/%{pfx}/usr/bin/apptrk

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
