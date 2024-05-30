%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir gst-plugins-bad

Summary         : Bad plugins for gstreamer framework
Name            : gst-plugins-bad
Version         : 0.10.7_1.7
Release         : 0
License         : LGPL
Vendor          : Sagem
Packager        : Ahmed Ben Achour
Group           : System Environment/Libraries
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
Source          : %{name}-%{version}.tar.gz
BuildRequires   : glib2, gstreamer-core, gst-plugins-base

%Description
%{summary}

%Prep
%setup -n %{buildsubdir}

%Build
export PKG_CONFIG="/opt/freescale/ltib/usr/bin/pkg-config --static"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

# required for zlib.h && faad
export CFLAGS="-I$DEV_IMAGE/usr/include -O2"
export LDFLAGS="-L$DEV_IMAGE/usr/lib"

if test "$PLAYER_SOURCE_HTTP_PRECONFIG" = "y" ; then
PLUGIN_NEON="--enable-neon"
else
PLUGIN_NEON="--disable-neon"
fi

if test "$PLAYER_SOURCE_MMS_PRECONFIG" = "y" ; then
PLUGIN_MMS="--enable-libmms"
else
PLUGIN_MMS="--disable-libmms"
fi

./configure NM=nm --prefix=%{_prefix} --build=%{_build} CC=${TOOLCHAIN_PREFIX}gcc --host=$CFGHOST \
  --enable-shared=yes --enable-static=no \
  --enable-aacparse \
  --disable-app \
  --disable-bayer \
  --disable-cdxaparse \
  --disable-deinterlace \
  --disable-dvdspu \
  --disable-festival \
  --disable-filter \
  --disable-flv \
  --disable-freeze \
  --disable-h264parse \
  --disable-interleave \
  --disable-librfb \
  --disable-modplug \
  --enable-mpeg4videoparse \
  --disable-mpegtsparse \
  --disable-mpegvideoparse \
  --disable-mve \
  --disable-nsf \
  --disable-nuvdemux \
  --disable-rawparse \
  --disable-replaygain \
  --enable-rtpmanager \
  --disable-sdp \
  --disable-selector \
  --disable-speed \
  --disable-speexresample \
  --disable-stereo \
  --disable-subenc \
  --disable-tta \
  --disable-videosignal \
  --disable-vmnc \
  --disable-y4m \
  --disable-rpath	\
  --disable-libtool-lock \
  --disable-valgrind	\
  --disable-examples \
  --with-plugins= \
  $PLUGIN_MMS \
  $PLUGIN_NEON \
  --disable-opengl \
  --disable-vcd \
  --disable-x \
  --disable-alsa \
  --disable-amrwb \
  --disable-bz2 \
  --disable-cdaudio \
  --disable-directfb \
  --disable-dts \
  --disable-divx \
  --disable-faac \
  --disable-faad \
  --disable-gsm \
  --disable-ivorbis \
  --disable-jack \
  --disable-ladspa \
  --disable-mpeg2enc \
  --disable-musepack \
  --disable-musicbrainz \
  --disable-mythtv \
  --disable-nas \
  --disable-timidity \
  --disable-wildmidi \
  --disable-sdl \
  --disable-sdltest \
  --disable-sndfile \
  --disable-soundtouch \
  --disable-spc \
  --disable-swfdec \
  --disable-theoradec \
  --disable-x264 \
  --disable-xvid \
  --disable-dvb	\
  --disable-fbdevsink \
  --disable-oss4 \
  --disable-metadata \
  --disable-ofa \
  --disable-mplex

make clean all

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10

make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.*a" | xargs rm -f
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstapp-0.10*
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/*.so

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/lib/*
