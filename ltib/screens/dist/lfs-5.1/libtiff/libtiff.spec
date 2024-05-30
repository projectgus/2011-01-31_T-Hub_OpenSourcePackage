%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A library of functions for manipulating TIFF format image files
Name            : libtiff
Version         : 3.8.2
Release         : 2
License         : Distributable
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Libraries
Source          : tiff-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n tiff-%{version}

%Build
export CC=gcc

./configure \
	--prefix=%{_prefix} --build=%{_build} --host=$CFGHOST \
	--disable-static --enable-shared \
	--with-zlib-include-dir=$DEV_IMAGE/usr/include \
	--with-zlib-lib-dir=$DEV_IMAGE/usr/lib \
	--with-jpeg-include-dir=$DEV_IMAGE/usr/include \
	--with-jpeg-lib-dir=$DEV_IMAGE/usr/lib
make

%Install
rm -rf $RPM_BUILD_ROOT
make install-strip DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/lib*.a
rm -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/lib*.la

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/usr/include
%{pfx}/usr/lib

