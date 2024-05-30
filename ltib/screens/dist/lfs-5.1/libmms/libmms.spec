%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : mms_stream
Name            : libmms
Version         : 0.4.5
Release         : 0
Packager        : Guillaume CHAUVEL
Group           : Development/Libraries
Source          : %{name}-%{version}.tar.gz
License         : LGPL
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n %{name}

%Build
export PKG_CONFIG="`which pkg-config` --static "
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"

./configure --prefix=%{_prefix} CC=gcc --host=$CFGHOST \
	--enable-shared=yes --enable-static=no
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f
strip $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.so*

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/%{_prefix}/include
%{pfx}/%{_prefix}/lib
