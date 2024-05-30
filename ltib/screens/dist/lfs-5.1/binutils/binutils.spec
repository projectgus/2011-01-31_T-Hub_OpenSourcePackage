%define pfx /opt/freescale/rootfs/%{_target_cpu} 

Summary         : A GNU collection of binary utilities.
Name            : binutils
Version         : 2.17
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Steve Papacharalambous
Group           : Development/Tools
Source          : %{name}-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 


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

rm -rf build-binutils
mkdir build-binutils
cd build-binutils
export CPP=$BUILDCPP
../configure \
    --host=${OPT_CFGHOST} --build=%{_build} --target=${OPT_CFGHOST} \
    --prefix=%{_prefix} --disable-nls --mandir=%{_mandir}
make CC_FOR_BUILD="${BUILDCC}" all

%Install
cd build-binutils
make CC_FOR_BUILD="${BUILDCC}" DESTDIR=${RPM_BUILD_ROOT}/%{pfx} install

# Remove libtool .la files.
find $RPM_BUILD_ROOT/%{pfx} -name \*.la -exec rm {} \;


%Clean
rm -rf ${RPM_BUILD_ROOT}


%Files
%defattr(-,root,root)
%{pfx}/*


