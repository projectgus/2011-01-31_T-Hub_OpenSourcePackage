%define pfx /opt/freescale/rootfs/%{_target_cpu} 

Summary         : Various compilers (C, C++, Objective-C, Java, ...)
Name            : gcc
Version         : 3.3.2
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Steve Papacharalambous
Group           : Development/Languages
Source          : %{name}-%{version}.tar.bz2
Patch0          : gcc-3.2.3-g++.exp.patch
Patch1          : gcc-3.3.2-arm-bigendian.patch
Patch2          : gcc-3.3.2-ccross-syslimits.patch
Patch3          : gcc-3.3-libstdc++-v3-dg.exp.patch
Patch4          : pr10392-1-test.patch
Patch5          : pr10412-1-test.patch
Patch6          : pr10589-1-test.patch
Patch7          : pr11162-1-test.patch
Patch8          : pr11587-1-test.patch
Patch9          : pr11608-fix.patch
Patch10         : pr11736-1-test.patch
Patch11         : pr11864-1-test.patch
Patch12         : pr13260-test.patch
Patch13         : pr9365-1-test.patch
Patch14         : sh-lib1funcs_sizeAndType.patch
Patch15         : sh-libgcc-hidden.patch
Patch16         : sh-pic-set_fpscr-gcc-3.3.2.patch
Patch17         : thunk3.patch
Patch18         : gcc-3.3.2-obstack_h.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1
%patch9 -p1
%patch10 -p1
%patch11 -p1
%patch12 -p1
%patch13 -p1
%patch14 -p1
%patch15 -p1
%patch16 -p1
%patch17 -p1
%patch18 -p0

%Build

# Temporary hack - stevep
# For building toolchain components an optimized version of the
# target is needed.  This should really be defined in the config
# area of ltib, and not done in the toolchain spec files.
# For now we assume that the optimised target can be derived by
# stripping the trailing "-" off the toolchain prefix, but this
# will not be true for all cases, eg when using uClibc toolchains.
OPT_CFGHOST=`echo ${TOOLCHAIN_PREFIX} | perl -n -e 's,-$,,;print'`

# Copy the crt files from the toolchain area to rootfs/usr/lib
# These are the crt files built by glibc (I think - stevep).
X_DIR=`echo ${TOOLCHAIN_PREFIX} | sed -e 's,-$,,'`
TC_PREFIX="`which ${TOOLCHAIN_PREFIX}gcc | perl -p -e 's,/bin/'${TOOLCHAIN_PREFIX}'gcc,,'`"
mkdir -p ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib
mkdir -p ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib/gcc/${X_DIR}/%{version}
cp ${TC_PREFIX}/${X_DIR}/lib/*.o ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib
cp ${TC_PREFIX}/${X_DIR}/lib/*.o ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib/gcc/${X_DIR}/%{version}

rm -rf build-gcc
mkdir build-gcc
cd build-gcc
export CPP=$BUILDCPP
    CC_FOR_BUILD="${BUILDCC}" \
    ../configure \
    --host=${OPT_CFGHOST} --build=%{_build} --target=${OPT_CFGHOST} \
    --prefix=%{_prefix}/local --disable-nls --enable-threads=posix \
    --enable-symvers=gnu --enable-__cxa_atexit --enable-c99 \
    --enable-long-long --enable-languages="c,c++" --enable-shared \
    --mandir=%{_mandir}
make -j1 all-build-libiberty
make -j1 all

%Install
cd build-gcc
make -j1 DESTDIR=${RPM_BUILD_ROOT}/%{pfx} install
find $RPM_BUILD_ROOT/%{pfx} -name \*.la -exec rm {} \;

%Clean
rm -rf ${RPM_BUILD_ROOT}


%Files
%defattr(-,root,root)
%{pfx}/*


