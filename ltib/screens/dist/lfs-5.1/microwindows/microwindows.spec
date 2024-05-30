%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Nano-X window display program and samples
Name            : microwindows
Version         : 0.90
Release         : 0
License         : MPL/GPL
Vendor          : Freescale
Packager        : Matt Waddel
Group           : Applications/System
Source          : microwindows-0.90.tar.gz
Patch1		: microwindows-0.90-coldfire.patch
Patch2		: microwindows-0.90-scripts.patch
Patch3          : microwindows-0.90-uclinux.patch
BuildRoot       : %{_tmppath}/%{name}-%{version}
Prefix          : %{pfx}

%Description
Microwindows (also known as nano-x) is a very small frame buffer
based X server. Its aim is to bring the features of modern windowing
environments to smaller devices and platforms. Also included are
several utility and demo programs.


%Prep
%setup
%patch1 -p1
%patch2 -p1
%patch3 -p1

%Build
cd src
make -j1 HOSTCC="$BUILDCC"

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/usr/nanox
mkdir -p $RPM_BUILD_ROOT/%{pfx}/usr/nanox/demos/tuxchess/images
cd src
cp -a bin/ $RPM_BUILD_ROOT/%{pfx}/usr/nanox
cp -a *.sh $RPM_BUILD_ROOT/%{pfx}/usr/nanox
cp -a demos/tuxchess/images/ $RPM_BUILD_ROOT/%{pfx}/usr/nanox/demos/tuxchess/ 
# Remove these build machine executables. Are they only need during build?
for i in convbmp makebmp convbdf
do
rm -f $RPM_BUILD_ROOT/%{pfx}/usr/nanox/bin/$i
done
rm -f $RPM_BUILD_ROOT/%{pfx}/usr/nanox/bin/*.gdb

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
