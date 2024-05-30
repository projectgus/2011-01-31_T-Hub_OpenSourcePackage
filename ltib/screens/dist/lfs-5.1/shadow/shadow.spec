%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Utilities for managing accounts and shadow password files
Name            : shadow
Version         : 4.0.3
Release         : 1
License         : BSD
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Base
Source          : shadow-4.0.3.tar.bz2
Patch0          : shadow-4.0.3-newgrp-fix.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1

%Build
./configure --prefix=%{_prefix} --libdir=%{_prefix}/lib --enable-shared
make

%Install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
mkdir -p $RPM_BUILD_ROOT/%{pfx}/etc
for i in limits login.access
do
    cp etc/$i $RPM_BUILD_ROOT/%{pfx}/etc/$i
done
for i in run log
do
mkdir -p $RPM_BUILD_ROOT/%{pfx}/var/$i
done
touch $RPM_BUILD_ROOT/%{pfx}/var/run/utmp 
for i in btmp lastlog wtmp
do
    touch     $RPM_BUILD_ROOT/%{pfx}/var/log/$i
    chmod 644 $RPM_BUILD_ROOT/%{pfx}/var/log/$i
done
chmod 644 $RPM_BUILD_ROOT/%{pfx}/var/run/utmp

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


