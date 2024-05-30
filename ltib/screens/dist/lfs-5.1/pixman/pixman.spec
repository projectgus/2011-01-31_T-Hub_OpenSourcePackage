%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Library
Name            : pixman
Version         : 0.10.0
Release         : 3
License         : MPL
Vendor          : SAGEM
Packager        : Anthony HERRERO
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
export CC=gcc
export PKG_CONFIG="`which pkg-config`"
export PKG_CONFIG_LIBDIR="$DEV_IMAGE/usr/lib/pkgconfig"
export PKG_CONFIG_PATH="$DEV_IMAGE/usr/lib/pkgconfig"
./configure \
	--build=%{_build} --host=$CFGHOST --prefix=%{_prefix}
cat <<EOF > test/Makefile 
#empty file due to pixman.spec
all install clean dist-clean test:
	@echo "FAKE $@"
EOF
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/usr/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
