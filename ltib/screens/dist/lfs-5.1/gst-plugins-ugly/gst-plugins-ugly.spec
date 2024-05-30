%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir gst-plugins-ugly

Summary         : Base plugins for gstreamer framework (including mad)
Name            : gst-plugins-ugly
Version         : 0.10.6_1.5
Release         : 0
License         : LGPL
Vendor          : Sagem
Packager        : Anthony HERRERO
Group           : System Environment/Libraries
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
BuildRequires   : glib2, gstreamer-core, gst-plugins-base
Source          : %{name}-%{version}.tar.gz

%Description
%{summary}

%Prep
rm -rf %{buildsubdir}
%setup -n %{buildsubdir}

%Build
export PKG_CONFIG="/opt/freescale/ltib/usr/bin/pkg-config --static"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

# required for libid3tag and mad because they don't create *.pc file
export CFLAGS="-I$DEV_IMAGE/usr/include -O2"
export LDFLAGS="-L$DEV_IMAGE/usr/lib"

PLUGINS=""

if test "$PLAYER_DEMUXER_ASF_PRECONFIG" = "y" -o "$PLAYER_DEPAYLOADER_ASF_PRECONFIG" = "y"; then
PLUGINS="$PLUGINS,asfdemux"
fi

if test "$PLAYER_CODEC_AC3_PRECONFIG" = "y" ; then
PLUGIN_A52DEC="--enable-a52dec"
else
PLUGIN_A52DEC="--disable-a52dec"
fi

./configure --prefix=%{_prefix} CC=gcc --host=$CFGHOST \
  --enable-shared=yes --enable-static=no	\
  --with-plugins=$PLUGINS \
  --disable-libtool-lock \
  --disable-rpath \
  --disable-nls \
  --disable-debug \
  --disable-valgrind \
  --with-check=no \
  $PLUGIN_A52DEC \
  --disable-amrnb \
  --disable-dvdread \
  --disable-lame \
  --disable-id3tag \
  --disable-mad \
  --disable-mpeg2dec \
  --disable-sidplay \
  --disable-synaesthesia \
  --disable-dvdnav \
  --disable-siddec
  

make clean all

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10

make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/lib/gstreamer-0.10/*.so

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/lib/*
