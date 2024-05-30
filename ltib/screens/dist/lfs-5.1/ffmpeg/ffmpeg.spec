%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : FFMPEG library
Name            : ffmpeg
Version         : 0.4.9.20071114T0500_2.2
Release         : 1
License         : LGPL
Vendor          : Sagem
Packager        : Anthony HERRERO
Group           : System Environment/Libraries
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
Source          : %{name}-%{version}.tar.gz

%Description
%{summary}

%Prep
%setup -n %{name}

%Build
export PKG_CONFIG="/opt/freescale/ltib/usr/bin/pkg-config --static"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"
export CFLAGS="-I$DEV_IMAGE/usr/include -g"
export LDFLAGS="-L$DEV_IMAGE/usr/lib -g"

FFMPEG_DECODER=""
FFMPEG_DEMUXER=""

if test "$PLAYER_DEMUXER_AVI_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=avi"
fi

if test "$PLAYER_DEMUXER_AC3_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=ac3"
fi

if test "$PLAYER_DEMUXER_MPEG_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=mpegps --enable-demuxer=mpegvideo"
fi

if test "$PLAYER_DEMUXER_MPEG_TS_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=mpegts --enable-demuxer=mpegtsraw"
fi

if test "$PLAYER_DEMUXER_MOV_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=mov"
fi

if test "$PLAYER_DEMUXER_MKV_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=matroska"
fi

if test "$PLAYER_DEMUXER_MP3_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=mp3"
fi

if test "$PLAYER_DEMUXER_OGG_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=ogg"
fi

if test "$PLAYER_DEMUXER_FLV_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=flv"
fi

if test "$PLAYER_DEMUXER_ASF_FFMPEG_PRECONFIG" = "y" ; then
FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=asf"
fi

#FFMPEG_DEMUXER="$FFMPEG_DEMUXER --enable-demuxer=mpc --enable-demuxer=mjpeg --enable-demuxer=m4v"

if test "$PLAYER_CODEC_VORBIS_PRECONFIG" = "y" ; then
FFMPEG_DECODER="$FFMPEG_DECODER --enable-decoder=vorbis"
fi

if test "$PLAYER_CODEC_WMA_FFMPEG_PRECONFIG" = "y" ; then
FFMPEG_DECODER="$FFMPEG_DECODER --enable-decoder=wmav1 --enable-decoder=wmav2"
fi

FFMPEG_DECODER="$FFMPEG_DECODER --enable-decoder=mp2 --enable-decoder=mp3"

chmod 755 version.sh
chmod 755 doc/texi2pod.pl
chmod +x configure

./configure --prefix=%{_prefix} \
	--cross-compile --cross-prefix= --arch=arm --cpu=arm1136jf-s \
	--enable-shared --disable-static \
	--disable-vhook --disable-ffmpeg --disable-ffserver --disable-ffplay \
	--disable-muxers --disable-encoders --disable-decoders --disable-bsfs --disable-demuxers \
	--enable-zlib \
	--enable-decoder=zlib \
	--disable-gpl \
	$FFMPEG_DECODER \
	$FFMPEG_DEMUXER \
	--enable-demuxer=rawvideo \
	--disable-network --disable-ipv6
make clean all

%Install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/include/*
%{pfx}/usr/lib/pkgconfig/*
%{pfx}/usr/lib/libav*
