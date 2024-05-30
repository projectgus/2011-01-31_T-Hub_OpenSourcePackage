%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir gst-plugins-base

Summary         : Base plugins for gstreamer framework (including alsa)
Name            : gst-plugins-base
Version         : 0.10.22_1.3
Release         : 0
License         : LGPL
Vendor          : Sagem
Packager        : Laurent Isenegger / Vincent Croze
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
export PKG_CONFIG="/opt/freescale/ltib/usr/bin/pkg-config --static"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

# /usr/lib/pkgconfig/alsa.pc doesn't work well with modified pkg-config so we force the CFLAGS, LDFLAGS
export CFLAGS="-I$DEV_IMAGE/usr/include -O2"
export LDFLAGS="-L$DEV_IMAGE/usr/lib"

./configure NM=nm \
	--prefix=%{_prefix} CC=gcc --host=$CFGHOST \
	--with-plugins=playback,audioconvert \
	--disable-valgrind \
	--disable-rpath  \
	--disable-nls \
	--disable-debug  \
	--disable-examples  \
	--disable-x  \
	--disable-xvideo  \
	--disable-xshm  \
	--disable-gst_v4l  \
	--disable-cdparanoia  \
	--disable-gnome_vfs  \
	--disable-libvisual  \
	--disable-ogg  \
	--disable-oggtest  \
	--disable-pango  \
	--disable-theora  \
	--disable-vorbis  \
	--disable-vorbistest  \
	--disable-freetypetest  \
	--disable-libtool-lock  \
	--enable-alsa

make

%Install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.*a" | xargs rm
# libgstrtsp,libgstsdp requiered for rtsp and rtp in gst-plugins-good
#rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstrtsp-0.10*
#rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstsdp-0.10*
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstcdda-0.10*
# libgstnetbuffer requiered for rtsp and rtp in gst-plugins-good
#rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstnetbuffer-0.10*
#rm $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstriff-0.10*
# libgstrtp, requiered for gst-plugins-good/qtdemux atm
#rm $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgstrtp-0.10*
#rm $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/libgsttag-0.10*
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/libgsttcp.*
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/libgstsubparse*
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/libgst*decodebin*
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/libgstplaybin*
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.so
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/*.so

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/include
%{pfx}/usr/lib/gstreamer-0.10/*.so*
%{pfx}/usr/lib/*.so*
%{pfx}/usr/lib/pkgconfig

