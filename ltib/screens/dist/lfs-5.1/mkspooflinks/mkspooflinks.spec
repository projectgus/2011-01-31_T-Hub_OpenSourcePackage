%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Setup for compiler spoofing (needed for interface lib/hdrs)
Name            : mkspooflinks
Version         : 2.3
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Steve Papacharalambous/Stuart Hughes
Group           : System Environment/Utilities
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup

%Build

%Install
rm -rf $RPM_BUILD_ROOT
sh mkspoof $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/spoof

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*

