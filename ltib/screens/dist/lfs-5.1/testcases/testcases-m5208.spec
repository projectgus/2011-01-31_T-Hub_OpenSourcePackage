%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define __os_install_post %{nil}

Summary         : Regression test cases
Name            : testcases-m5208
Version         : 1.0
Release         : 0
License         : GPL
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
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


