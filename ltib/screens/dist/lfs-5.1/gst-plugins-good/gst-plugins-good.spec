%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir gst-plugins-good

Summary         : GStreamer Plugins Good
Name            : gst-plugins-good
Version         : 0.10.14_1.2
Release         : 0
License         : LGPL
Vendor          : Sagem
Packager        : Guillaume CHAUVEL
Group           : System Environment/Libraries
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
Source          : %{name}-%{version}.tar.gz

%Description
%{summary}

%Prep
rm -rf %{buildsubdir}
%setup -n %{buildsubdir}

%Build
export PKG_CONFIG="`which pkg-config` --static "
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

export CFLAGS="-I$DEV_IMAGE/usr/include -O2"
export LDFLAGS="-L$DEV_IMAGE/usr/lib"
export LIBPNG_LIBS="-lz"

PLUGINS=""

if [ "$PLAYER_SOURCE_RTSP_PRECONFIG" = "y" ] ; then
PLUGINS="$PLUGINS,rtsp,rtp,udp"
fi

if [ "$PLAYER_DEMUXER_AVI_STREAMING_PRECONFIG" = "y" ] ; then
PLUGINS="$PLUGINS,avi"
fi

if [ "$PLAYER_DEMUXER_WAV_PRECONFIG" = "y" ] ; then
PLUGINS="$PLUGINS,wavparse"
fi

if [ "$PLAYER_CODEC_JPEG_PRECONFIG" = "y" ] ; then
PLUGINS="$PLUGINS,jpeg"
JPEG="--enable-jpeg"
else
JPEG="--disable-jpeg"
fi

PLUGINS="$PLUGINS,equalizer"

if [[ "$PLAYER_CODEC_PNG_PRECONFIG" = "y" || "$PLAYER_CODEC_BMP_PRECONFIG" = "y" || "$PLAYER_CODEC_GIF_PRECONFIG" = "y" || "$PLAYER_CODEC_TIFF_PRECONFIG" = "y" || "$PLAYER_CODEC_ICO_PRECONFIG" = "y" ]] ; then
GDK_PIXBUF="--enable-gdk_pixbuf"
else
GDK_PIXBUF="--disable-gdk_pixbuf"
fi

./configure NM=nm --prefix=%{_prefix} CC=gcc --host=$CFGHOST \
  --enable-shared=yes --enable-static=no \
  --with-check=no \
  --with-plugins=$PLUGINS \
  --disable-rpath	\
  --disable-nls \
  --disable-valgrind \
  --disable-examples \
  --enable-external \
  --disable-schemas-install \
  --disable-gconftool \
  --disable-directdraw \
  --disable-directsound \
  --disable-oss \
  --disable-sunaudio \
  --disable-osx_audio \
  --disable-osx_video \
  --disable-gst_v4l2 \
  --disable-x \
  --disable-xshm \
  --disable-xvideo \
  --disable-aalib \
  --disable-aalibtest \
  --disable-annodex \
  --disable-cairo \
  --disable-cdio \
  --disable-esd \
  --disable-esdtest \
  --disable-flac \
  --disable-gconf \
  $GDK_PIXBUF \
  --disable-hal \
  $JPEG \
  --disable-libcaca \
  --disable-libdv \
  --disable-libpng \
  --disable-dv1394 \
  --disable-shout2 \
  --disable-shout2test \
  --disable-speex \
  --disable-taglib \
  --disable-wavpack \
  --disable-zlib

make clean all

%Install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.*a" | xargs rm -f
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/*.so
%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/%{_prefix}/lib/gstreamer-0.10/*.so*
