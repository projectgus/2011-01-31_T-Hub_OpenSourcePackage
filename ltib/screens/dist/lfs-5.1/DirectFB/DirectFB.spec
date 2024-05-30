%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : DirectFB is a graphics library for embedded systems
Name            : DirectFB
Version         : 0.9.24
Release         : 1
License         : LGPL
Vendor          : Freescale
Packager        : WMSG
Group           : System Environment/Base
Source          : %{name}-%{version}.tar.gz
Patch1          : DirectFB-0.9.24-ppcasm.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch1 -p1 

%Build
export FREETYPE_CONFIG=${DEV_IMAGE}/usr/bin/freetype-config
export FREETYPE_CFLAGS=`${FREETYPE_CONFIG} --prefix=${DEV_IMAGE}/%{_prefix} --cflags`
export FREETYPE_LIBS=`${FREETYPE_CONFIG} --prefix=${DEV_IMAGE}/%{_prefix} --libs`
./configure --enable-shared --host=$CFGHOST --build=%{_build} \
            --prefix=%{_prefix} --with-gfxdrivers=none \
            --disable-x11 --enable-fbdev --enable-video4linux2 --disable-sdl
make KHDR=$DEV_IMAGE/usr/src/linux/include

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib -name \*.la | xargs rm -f

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
