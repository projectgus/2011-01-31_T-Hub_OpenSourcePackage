%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Clutter OpenGL MMI Toolkit
Name            : clutter-core
Version         : 0.8.0_1.7
Release         : 1
License         : LGPL
Vendor          : Sagem
Packager        : Anthony HERRERO
Group           : System Environment/Libraries
Source          : clutter-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}-%{version}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n clutter

%Build
export PKG_CONFIG="`which pkg-config` --static "
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

export CC=gcc
export CFLAGS="-I$DEV_IMAGE/usr/include -DAPIENTRY= -DK_SAGEM_HOMESCREEN=1"
export CPPFLAGS="-I$DEV_IMAGE/usr/include -DAPIENTRY="
export LDFLAGS="-L$DEV_IMAGE/usr/lib"

#export TSLIB_CFLAGS="-I$DEV_IMAGE/usr/include"
#export TSLIB_LIBS="-I$DEV_IMAGE/usr/lib -lts"
export CLUTTER_DEPS_LIBS="-L$DEV_IMAGE/usr/lib -lxml2 -lfontconfig -lexpat -lfreetype -lpangocairo-1.0 -lpangoft2-1.0 -lpango-1.0 -lcairo -lpixman-1 -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lgthread-2.0 -lgdk_pixbuf-2.0 -lpng -ljpeg -ltiff -lz"

./configure \
	--prefix=%{_prefix} --build=%{_build} --host=$CFGHOST \
	--enable-shared --disable-static \
	--enable-manual=no \
	--with-flavour=eglnative \
	--without-x

make

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install

mkdir -p $RPM_BUILD_ROOT/%{pfx}/opt/clutter
install -m 755 tests/.libs/* $RPM_BUILD_ROOT/%{pfx}/opt/clutter/
install -m 644 tests/*.png $RPM_BUILD_ROOT/%{pfx}/opt/clutter/
install -m 644 tests/*.json $RPM_BUILD_ROOT/%{pfx}/opt/clutter/

cp -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-0.8.pc $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-0.7.pc
cp -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-0.8.pc $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-0.6.pc

cp -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-eglnative-0.8.pc $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-eglnative-0.7.pc
cp -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-eglnative-0.8.pc $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/clutter-eglnative-0.6.pc

rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
