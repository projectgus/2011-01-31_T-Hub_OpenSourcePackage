%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A library of functions used by GDK, GTK+, and many applications
Name            : glib2
Version         : 2.14.6
Release         : 1
License         : LGPL
Vendor          : Freescale
Packager        : Stuart Hughes/Kurt Mahan
Group           : System Environment/Libraries
Source          : glib-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n glib-%{version}

%Build
export CFLAGS="-O2"

# prevent configure from trying to compile and
# run test binaries for the target.
glib_cv_stack_grows=no \
glib_cv_uscore=no \
ac_cv_func_posix_getpwuid_r=yes \
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib -name "*.la" | xargs rm -f
STRIP=${TOOLCHAIN_PREFIX}strip
$STRIP $RPM_BUILD_ROOT/%{pfx}/usr/lib/*.so

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/usr/include
%{pfx}/usr/lib/glib-2.0/include
%{pfx}/usr/lib/*.so*
%{pfx}/usr/lib/pkgconfig
