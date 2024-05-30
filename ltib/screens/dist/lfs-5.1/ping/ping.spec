%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Standard ping software ported from uClinux
Name            : ping
Version         : 1.0
Release         : 0
License         : BSD
Vendor          : Freescale
Packager        : Matt Waddel
Group           : Applications/Test
URL		: http://cvs.uclinux.org/cgi-bin/cvsweb.cgi/uClinux-dist/user/ping/
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
make

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp ping $RPM_BUILD_ROOT/%{pfx}/usr/bin/

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/bin/ping
