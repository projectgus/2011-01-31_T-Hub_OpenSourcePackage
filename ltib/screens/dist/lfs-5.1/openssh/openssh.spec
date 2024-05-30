%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : The OpenSSH implementation of SSH protocol versions 1 and 2.
Name            : openssh
Version         : 4.3p2
Release         : 1
License         : BSD
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/Internet
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
LD=gcc ./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}  --with-zlib=$DEV_IMAGE/usr --with-ssl-dir=$DEV_IMAGE/usr --with-ldflags="-L$DEV_IMAGE/lib" --config-cache

make -j1 sysconfdir=%{_sysconfdir}/ssh

%Install
rm -rf $RPM_BUILD_ROOT
make -j1 install sysconfdir=%{_sysconfdir}/ssh DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
