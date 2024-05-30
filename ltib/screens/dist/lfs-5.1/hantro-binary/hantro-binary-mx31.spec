%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Hantro MPEG4 encoder and decoder binaries
Name            : hantro-binary-mx31
Version         : 2.0
Release         : 0
License         : Proprietary
Vendor          : Hantro Products Oy.
Packager        : Duck
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}-%{version}
Prefix          : %{pfx}
%Description
This package provides proprietary libraries, header files, documentation and
a decoder binary built from the Hantro SDK. The libraries implement
APIs that help with MPEG4 encoding and decoding. The decoder library is a
software implementation, while the encoder library interfaces with
the hardware MPEG4 encoder block, thru the mxc_hmp4e kernel module.

%{summary}

%Prep
%setup

%Build

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp decoder/dec  $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp decoder/dec_noout  $RPM_BUILD_ROOT/%{pfx}/usr/bin

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
