%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Peak Linux CAN driver
Name            : peak-linux-driver
Version         : 3.17
Release         : 1
License         : GPL
Vendor          : Peak Systems + Freescale patch
Packager        : John Rigby
Group           : System Environment/Libraries
URL             : http://www.peak-system.com/linux
Source          : %{name}-%{version}.tgz
Patch0          : %{name}-%{version}-mpc5200.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1

%Build
make KERNEL_LOCATION=$TOP/rpm/BUILD/linux-2.6.10 KERNSRC=$TOP/rpm/BUILD/linux-2.6.10 CROSS_COMPILE=$TOOLCHAIN_PREFIX ARCH=ppc


%Install
install -d $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp test/bitratetest $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp test/transmitest $RPM_BUILD_ROOT/%{pfx}/usr/bin
cp test/receivetest $RPM_BUILD_ROOT/%{pfx}/usr/bin
install -d $RPM_BUILD_ROOT/%{pfx}/usr/lib
cp -a lib/libpcan.so* $RPM_BUILD_ROOT/%{pfx}/usr/lib
install -d $RPM_BUILD_ROOT/%{pfx}/usr/lib/can
cp test/transmit.txt $RPM_BUILD_ROOT/%{pfx}/usr/lib/can
mkdir -p $RPM_BUILD_ROOT/%{pfx}/dev
echo "%attr(0640, root, root) %dev(c, 254, 40) %{pfx}/dev/pcan40" > manifest
echo "%attr(0640, root, root) %dev(c, 254, 41) %{pfx}/dev/pcan41" >> manifest
install -d $RPM_BUILD_ROOT/%{pfx}/lib/modules/2.6.11.7/can
cp driver/pcan.ko $RPM_BUILD_ROOT/%{pfx}/lib/modules/2.6.11.7/can


%Clean
rm -rf $RPM_BUILD_ROOT

%Files -f manifest
%defattr(-,root,root)
%{pfx}/*
