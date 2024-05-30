%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Synchronizes system time using the Network Time Protocol (NTP)
Name            : ntpclient
Version         : 2003_194
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
URL             : http://doolittle.faludi.com/ntpclient/
Group           : System Environment/Base
Source          : %{name}_%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n ntpclient

%Build
# adjtimex could be built, but we're only interested in setting the time
make

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/bin
cp ntpclient $RPM_BUILD_ROOT/%{pfx}/bin/
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/bin/*

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
