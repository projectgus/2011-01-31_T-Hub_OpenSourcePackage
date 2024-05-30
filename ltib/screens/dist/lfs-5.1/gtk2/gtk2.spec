%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : The GIMP ToolKit (GTK+), a library for creating GUIs for X
Name            : gtk2
Version         : 2.4.9.p1
Release         : 1
License         : LGPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Daemons
Source          : gtk2-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n %{name}
sed -i "s/newlib_search_path=$/#SAGEM: newlib_search_path=/" ltmain.sh

%Build
export PKG_CONFIG="`which pkg-config`"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

./configure \
	--prefix=%{_prefix} --build=%{_build} --host=$CFGHOST \
	--enable-shared \
	--disable-static \
	--disable-shadowfb \
	--enable-fbmanager \
	--disable-glibtest \
	--with-xinput=no \
	--with-gdktarget=linux-fb \
	--disable-modules \
	--with-included-loaders=bmp,gif,jpeg,ico,png,tiff,pcx,tga \
	--without-x

make -C gdk-pixbuf
make -C gdk

%Install
rm -rf $RPM_BUILD_ROOT
make -C gdk-pixbuf DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
make -C gdk DESTDIR=$RPM_BUILD_ROOT/%{pfx} install

mkdir -p $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig
cat << _EOF > $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/gdk-pixbuf-2.0.pc
prefix=/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include/gtk-2.0

Name: gdk-pixbuf
Description: Decoding library
Version: 1.0
Libs: -L\${libdir} -lgdk_pixbuf-2.0 -lpng -ltiff -ljpeg
Cflags: -I\${includedir}
_EOF

cat << _EOF > $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig/gtk+-2.0.pc
prefix=/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include
requires=gdk-pixbuf-2.0

Name: gtk+-2.0
Description: Fake
Version: 2.4.9
Libs: -L\${libdir}
Cflags: -I\${includedir}/gtk-2.0
Requires: \${requires}
_EOF

find $RPM_BUILD_ROOT/%{pfx}/ -name "*.la" | xargs rm -f

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/include/*
%{pfx}/usr/lib/*
