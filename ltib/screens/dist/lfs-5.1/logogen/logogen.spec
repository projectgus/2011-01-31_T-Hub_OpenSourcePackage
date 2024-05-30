%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir logogen

Summary         : A utility for managing logos
Name            : logogen
Version         : 1.6
Release         : 1
License         : Sagem Property
Vendor          : Sagem Communications
Packager        : g094870
Group           : Applications/logogen
#Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep

%Build


%Install
cd ../../
mkdir -p $RPM_BUILD_ROOT/%{pfx}/boot
install -m 644 bin/logo.bin $RPM_BUILD_ROOT/%{pfx}/boot
install -m 644 bin/logo_custom_clut224.ppm $RPM_BUILD_ROOT/%{pfx}/boot

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*


