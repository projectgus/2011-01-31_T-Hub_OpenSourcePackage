%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : i2c tools
Name            : i2c-tools
Version         : 2.8.1
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Tools
URL             : http://secure.netroedge.com/~lm78/index.html
Source          : i2c_tools-2jun05.tar.gz
Patch1          : i2c-tools-device.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n i2c-tools
%patch1 -p1

%Build
make

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/sbin
for i in i2cdetect i2cset i2cdump
do
    cp -a $i $RPM_BUILD_ROOT/%{pfx}/sbin/$i
done

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
