%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir gstreamer

Summary         : GStreamer Core
Name            : gstreamer-core
Version         : 0.10.22_1.3
Release         : 0
License         : LGPL
Vendor          : Sagem
Packager        : Guillaume CHAUVEL
Group           : System Environment/Libraries
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
Source          : gstreamer-%{version}.tar.gz

%Description
%{summary}

%Prep
rm -rf %{buildsubdir}
%setup -n %{buildsubdir}

%Build
export PKG_CONFIG="`which pkg-config` --static "
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

export CFLAGS="-O2"

./configure NM=nm \
	ac_cv_func_register_printf_function=no \
	ac_cv_func_mmap_fixed_mapped=yes \
	--prefix=%{_prefix} --build=%{_build} CC=gcc --host=$CFGHOST \
	--enable-registry  \
	--enable-parse  \
	--disable-loadsave \
	--disable-valgrind  \
	--with-check=no  \
	--disable-libtool-lock  \
	--disable-dependency-tracking  \
	--disable-nls  \
	--disable-net  \
	--disable-examples  \
	--disable-tests  \
	--disable-failing-tests  \
	--disable-debug  \
	--disable-valgrind  \
	--disable-rpath
make

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}
make DESTDIR=$RPM_BUILD_ROOT/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f
strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.so*
strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/gstreamer-0.10/*.so*

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/bin
%{pfx}/usr/include
%{pfx}/usr/lib/gstreamer-0.10/*.so*
%{pfx}/usr/lib/*.so*
%{pfx}/usr/lib/pkgconfig
