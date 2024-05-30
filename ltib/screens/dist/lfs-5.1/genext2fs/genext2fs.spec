%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Tools for the second extended (ext2) filesystem
Name            : genext2fs
Version         : 1.3
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/System
Source          : http://ftp.debian.org/debian/pool/main/g/genext2fs/%{name}_%{version}.orig.tar.gz
Patch0          : genext2fs-01-cvs-updates.patch
Patch1          : genext2fs-02-nosquash_devtable.patch
Patch2          : genext2fs-makefile_in.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n %{name}-%{version}.orig
%patch0 -p1
%patch1 -p1
%patch2 -p1

%Build
# There is a race condition, both configure and aclocal.m4
# get touched, sometimes this leads to configure being rebuilt
# which fail with: automake-1.8: command not found
sh ./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --mandir=%{_mandir}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
