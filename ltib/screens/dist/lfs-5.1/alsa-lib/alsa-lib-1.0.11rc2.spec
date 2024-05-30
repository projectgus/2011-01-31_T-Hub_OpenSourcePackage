%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A libraries for ALSA (Advanced Linux Sound Architecture)
Name            : alsa-lib
Version         : 1.0.11rc2.p5
# 1.0.11rc2 is compatible with linux 2.6.16 kernels
#   (see linux/include/sound/version.h)
Release         : 0
License         : LGPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : System Environment/Libraries
Source          : alsa-lib-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n alsa-lib

%Build
if false; then
ULOG_COMPILE=-DU_LOG_ON
ULOG_LINK=-lulog
else
ULOG_COMPILE=-UDU_LOG_ON
fi
#PATH=/usr/bin:/bin/usr/lib:$PATH

touch * */* */*/*

./configure --with-debug=no --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} CC=gcc AR=ar LD=ld CXX=c++ STRIP=strip "CFLAGS=-I$DEV_IMAGE/usr/include/ -I$DEV_IMAGE/usr/src/linux/include/ $ULOG_COMPILE" LIBS="-L$DEV_IMAGE/usr/lib" LDFLAGS="$ULOG_LINK"
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/alsa-lib/*/*.la
#strip
strip ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/*
strip ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libasound.so.2.0.0
strip ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/alsa-lib/smixer/*.so

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/usr/bin/*
%{pfx}/usr/include/*
%{pfx}/usr/lib/*
%{pfx}/usr/share/alsa/alsa.conf
%{pfx}/usr/share/alsa/pcm/*
%{pfx}/usr/share/alsa/sndo-mixer.alisp
%{pfx}/usr/share/alsa/cards/aliases.*
