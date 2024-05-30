%define pfx /opt/freescale/rootfs/%{_target_cpu} 

Summary         : Various compilers (C, C++, Objective-C, Java, ...)
Name            : gcc
Version         : 3.4.3
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Steve Papacharalambous
Group           : Development/Languages
Source          : %{name}-%{version}.tar.bz2
Patch0          : gcc-3.4.0-arm-bigendian.patch
Patch1          : gcc-3.4.0-arm-lib1asm.patch
Patch2          : gcc-3.4.0-arm-nolibfloat.patch
Patch3          : gcc-3.4.3-ccross-syslimits.patch
Patch4          : pr15068-fix.patch
Patch5          : pr16201-fix.patch
Patch6          : gcc-3.4.3-libiberty-cp-demangle.h.patch
Patch7          : gcc-3.4.3-make-3.81.patch
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

%Build
# Temporary hack - stevep
# For building toolchain components an optimized version of the
# target is needed.  This should really be defined in the config
# area of ltib, and not done in the toolchain spec files.
# For now we assume that the optimised target can be derived by
# stripping the trailing "-" off the toolchain prefix, but this
# will not be true for all cases, eg when using uClibc toolchains.
if [ ${TOOLCHAIN_PREFIX} = "arm_v6_vfp_le-" -o -n "$UCLIBC" ]
then
    OPT_CFGHOST=$CFGHOST
else
    OPT_CFGHOST=`echo ${TOOLCHAIN_PREFIX} | perl -n -e 's,-$,,;print'`
fi
if [  -n "$UCLIBC" ]
then
   ENABLE_LANGUAGES="c"
else
   ENABLE_LANGUAGES="c,c++"
fi

rm -rf build-gcc
mkdir build-gcc
cd build-gcc
export CPP=$BUILDCPP
    CC_FOR_BUILD="${BUILDCC}" \
    ../configure \
    --host=${OPT_CFGHOST} --build=%{_build} --target=${OPT_CFGHOST} \
    --prefix=%{_prefix}/local --disable-nls --enable-threads=posix \
    --enable-symvers=gnu --enable-__cxa_atexit --enable-c99 \
    --enable-long-long --enable-languages="$ENABLE_LANGUAGES" --enable-shared \
    --mandir=%{_mandir}
make -j1 all-build-libiberty
make -j1 all

%Install
rm -rf $RPM_BUILD_ROOT


if [ ${TOOLCHAIN_PREFIX} = "arm_v6_vfp_le-" -o -n "$UCLIBC" ]
then
    OPT_CFGHOST=$CFGHOST
else
    OPT_CFGHOST=`echo ${TOOLCHAIN_PREFIX} | perl -n -e 's,-$,,;print'`
fi
if [ "$TOOLCHAIN_TYPE" = "64" ]
then
   SLIBS="lib64"
else
   SLIBS="lib"
fi
# Copy the crt files from the toolchain area to rootfs/usr/local/lib
# These are the crt files built by glibc (I think - stevep).
X_DIR=`echo ${OPT_CFGHOST} | sed -e 's,-$,,'`
TC_PREFIX="`which ${TOOLCHAIN_PREFIX}gcc | perl -p -e 's,/bin/'${TOOLCHAIN_PREFIX}'gcc,,'`"
mkdir -p ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib/{,gcc/${X_DIR}/%{version}}
cp ${TC_PREFIX}/${X_DIR}/${SLIBS}/*.o ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib
cp ${TC_PREFIX}/${X_DIR}/${SLIBS}/*.o ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/lib/gcc/${X_DIR}/%{version}

cd build-gcc
make -j1 DESTDIR=${RPM_BUILD_ROOT}/%{pfx} install

# Remove the c++ pre-compiled header files.
X_DIR=`echo ${TOOLCHAIN_PREFIX} | sed -e 's,-$,,'`
rm -f ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/include/c++/%{version}/${X_DIR}/bits/stdc++.h.gch/O[02]g

# Remove libtool .la files
find $RPM_BUILD_ROOT/%{pfx} -name \*.la -exec rm {} \;

# add a symlink for cc
ln -s gcc ${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}/local/bin/cc

%Clean
rm -rf ${RPM_BUILD_ROOT}


%Files
%defattr(-,root,root)
%{pfx}/*


