%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Font configuration and customization library
Name            : fontconfig
Version         : 2.4.2
Release         : 1
License         : MIT
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix		: %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
export PKG_CONFIG="`which pkg-config` --static "
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"
# we don't have docbook-utils in the distribution
echo 'ac_cv_prog_HASDOCBOOK=${ac_cv_prog_HASDOCBOOK=no}' > config.cache

export EXEEXT_FOR_BUILD= 
#export CC=${TOOLCHAIN_PREFIX}gcc
export CFLAGS="-I$DEV_IMAGE/usr/include/freetype2 -include $DEV_IMAGE/usr/include/freetype2/freetype/config/ftheader.h"

./configure \
	--prefix=%{_prefix} --host=$CFGHOST --build=%{_build} \
	--cache-file config.cache --with-arch=arm-linux \
	--disable-static --enable-shared \
	--with-freetype-config=$DEV_IMAGE/usr/bin/freetype-config \
	--with-expat-includes=$DEV_IMAGE/usr/include \
	--with-expat-lib=$DEV_IMAGE/usr/lib \
	--with-cache-dir=/tmp/fontconfig

export SAVE_PATH=$PATH


export PATH=/usr/bin:/bin
make -C fc-lang
make -C fc-case
make -C fc-glyphname
make -C fc-arch

export PATH=$SAVE_PATH

AM_MAKEFLAGS="--assume-old=Makefile --assume-old=fc-lang  --assume-old=fclang.h --assume-old=fc-lang.o" make 

# patch fontconfig.pc
sed -i "s,^\(Libs:.*\)$,\1 -lexpat," fontconfig.pc


%Install
rm -rf $RPM_BUILD_ROOT
AM_MAKEFLAGS="--assume-old=Makefile --assume-old=fc-lang  --assume-old=fclang.h --assume-old=fc-lang.o" make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install 
perl -pi -e 's,^</fontconfig>,
<dir>%{_prefix}/X11R6/lib/X11/fonts/TTF</dir>
<dir>%{_prefix}/X11R6/lib/X11/fonts/Type1</dir>
<dir>%{_prefix}/X11R6/lib/X11/fonts/truetype</dir>
</fontconfig>
,;
        ' $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/etc/fonts/local.conf

rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/usr/bin/*
%{pfx}/usr/etc/*
%{pfx}/usr/include/*
%{pfx}/usr/lib/*
