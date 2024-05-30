
%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : platform specific unit tests for mxc platform
Name            : mxc-misc
Version         : 20070112
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : Test
Source          : misc-mx-%{version}.tar.gz
Patch0		: misc-linux_2.6.19_fixup-2.patch
Patch1		: misc-remove_i2c.patch
Patch2		: misc-20070205-rm_threshold.patch
Patch3		: misc-20070209.patch
Patch4		: misc-SDCaa01599_pm_unit_test_code_compiling_error-2.patch
Patch5		: misc-SDCaa01701_shutdown_VPU_issue.patch
Patch6		: misc-SDCaa01883_Update_VPU_lib.patch
Patch7		: misc-20070408-define_I2C_NR.patch
Patch8		: misc-20070417-tls-2.patch
Patch9		: misc-20070511-autoconf.patch

BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n misc
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

%Build
if [ -z "$PKG_KERNEL_KBUILD_PRECONFIG" ]
then
      KERNELDIR="$PWD/../linux-2.6.19.2"
      KBUILD_OUTPUT=$KERNELDIR
else
      KERNELDIR="$PKG_KERNEL_PATH_PRECONFIG"
      KBUILD_OUTPUT="$(eval echo ${PKG_KERNEL_KBUILD_PRECONFIG})"
fi
PLATFORM_UPPER="$(echo $PLATFORM | tr '[:lower:]' '[:upper:]')"
make PLATFORM=$PLATFORM_UPPER CROSS_COMPILE= KBUILD_OUTPUT=$KBUILD_OUTPUT LINUXPATH=$KERNELDIR

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/unit_tests
PLATFORM_UPPER="$(echo $PLATFORM | awk '{print toupper($0)}')"
cp -rf platform/$PLATFORM_UPPER/* $RPM_BUILD_ROOT/%{pfx}/unit_tests/

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(755,root,root)
%{pfx}/*
