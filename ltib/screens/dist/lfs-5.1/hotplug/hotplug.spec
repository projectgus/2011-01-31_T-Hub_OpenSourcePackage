%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir hotplug

Summary         : A helper application which loads modules for USB devices
Name            : hotplug-screens
Version         : 2.12
Release         : 0
License         : GPL
Vendor          : Freescale
Packager        : Crevoisier Louis
Group           : System Environment/Kernel
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup  -n %{buildsubdir}

%Build
make

%Install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
