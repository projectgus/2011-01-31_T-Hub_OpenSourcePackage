%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Library for layout and rendering of internationalized text.
Name            : pango
Version         : 1.20.2
Release         : 2
License         : LGPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
export CC=gcc
export PKG_CONFIG="`which pkg-config`"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"
export FREETYPE_LIBS="-L$DEV_IMAGE/usr/lib -lfreetype -lz -lxml2 -lfontconfig"
export FONTCONFIG_LIBS="-L$DEV_IMAGE/usr/lib -lexpat"
export CAIRO_LIBS="-L$DEV_IMAGE/usr/lib -lcairo -lpixman-1 -lpng"
./configure \
	--build=%{_build} --host=$CFGHOST --prefix=%{_prefix} \
	--with-included-modules=yes \
	--without-x
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/include/*
%{pfx}/usr/lib/*
