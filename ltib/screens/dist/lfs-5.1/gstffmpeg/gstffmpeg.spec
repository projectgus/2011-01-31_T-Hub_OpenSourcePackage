%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir gstffmpeg

Summary         : FFMPEG plugins for gstreamer framework 
Name            : gstffmpeg
Version         : 0.10.7_1.7
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
export CFLAGS="-O2"

./configure NM=nm \
	--prefix=%{_prefix} --host=$CFGHOST \
	--enable-shared=yes --enable-static=no --disable-gpl \
	--with-system-ffmpeg

make clean all

%Install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/lib/gstreamer-0.10/*.so

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/lib/gstreamer-0.10/*.so*

