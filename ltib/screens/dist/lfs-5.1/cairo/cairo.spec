%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : 2D Library
Name            : cairo
Version         : 1.6.4
Release         : 2
License         : MPL
Vendor          : SAGEM
Packager        : Anthony HERRERO
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
export FREETYPE_LIBS="-L$DEV_IMAGE/usr/lib -lfreetype"
export FONTCONFIG_LIBS="-L$DEV_IMAGE/usr/lib -lexpat -lfontconfig -lz -lxml2 -lfreetype"
./configure \
	--build=%{_build} --host=$CFGHOST --prefix=%{_prefix} \
	--enable-xlib=no \
	--enable-xlib-xrender=no \
	--enable-xcb=no \
	--enable-quartz=no \
	--enable-quartz-font=no \
	--enable-quartz-image=no \
	--enable-win32=no \
	--enable-win32-font=no \
	--enable-os2=no \
	--enable-beos=no \
	--enable-png=yes \
	--enable-glitz=no \
	--enable-directfb=no \
	--enable-freetype=yes \
	--enable-ps=no \
	--enable-pdf=no \
	--enable-svg=no \
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
