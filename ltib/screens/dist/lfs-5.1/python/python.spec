%define pfx /opt/freescale/rootfs/%{_target_cpu}

%define version 2.2.1
%define release 1

Summary         : An interpreted object-oriented programming language.
Name            : python
Version         : %{version}
Release         : %{release}
License         : OSI Approved Python License
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Languages
Source          : Python-%{version}.tgz
Patch0          : python-cross-compile.diff
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n Python-%{version}
%patch0 -p1

%Build
#
# Note: This cross compile method is adapted from: 
#       http://www.ailis.de/~k/docs/crosscompiling/python.php
#
# build a version for the machine we are building on, before we
# build for the target
#
ORIG_PATH=$PATH
export PATH=$UNSPOOF_PATH
./configure
make python Parser/pgen
mv python hostpython
mv Parser/pgen Parser/hostpgen
make distclean


export PATH=$ORIG_PATH

./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --enable-shared --without-libdb --without-ssl

make HOSTPYTHON=./hostpython  HOSTPGEN=./Parser/hostpgen

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%%{pfx}/{_prefix}
make prefix=$RPM_BUILD_ROOT/%{pfx}/%{_prefix} HOSTPYTHON=./hostpython \
     CROSS_COMPILE=yes install  

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
