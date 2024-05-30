%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define __os_install_post %{nil}

Summary         : Hello World test package
Name            : helloworld
Version         : 1.0
Release         : 1
License         : Public Domain, not copyrighted
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/Test
Source          : %{name}-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
gcc -g -o hello hello.c
g++ -g -o hellocpp hello.cpp

%Install
rm -rf $RPM_BUILD_ROOT
for i in bin src/helloworld
do
    mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/$i
done
cp hellocpp hello $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin
cp hello.c hello.cpp $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/src/helloworld

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*

