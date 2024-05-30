%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Gdb - GNU Source level debugger for C, C++
Name            : gdb
Version         : 6.3.50.20051117
Release         : 0
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes/Matt Waddel
Group           : Development/Debuggers
Source          : gdb-6.3.50.20051117.tar.bz2
Source1         : m68k-bdm-1.3.0.tar.gz
Source2         : 5485.gdb
Patch0          : gdb-m68k-fp64bits.patch
Patch1          : gdb-6.3-tcsetpgrp-1.patch
Patch2          : gdb-6.3-arm-native.patch
Patch3          : gdb-cs-20060508-nommu.patch
Patch4          : gdb-cs-20060907-nommu.patch
Patch5          : gdb-cs-20060913-nommu.patch
Patch6          : gdb-cs-exit-nommu.patch
Patch7          : m68k-bdm-1.3.1.patch
Patch8          : m68k-bdm-build.patch
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
cd ${RPM_BUILD_DIR}/%{name}-%{version}
tar zxvf %{SOURCE1}
%patch7
cp %{SOURCE2} .
%patch8 -p1

%Build
ORIG_PATH=$PATH
# This is the optimised host type.  Needed to prevent infering a native build
if [ ${TOOLCHAIN_PREFIX} = "arm_v6_vfp_le-" ]
then
        OPT_CFGHOST=arm-linux
else
        OPT_CFGHOST=`echo ${TOOLCHAIN_PREFIX} | perl -n -e 's,-$,,;print'`
fi

# do something (the least costly)
if [ -z "$PKG_GDB_CROSS_WANT_ED$PKG_GDB_SERVER_WANT_ED$PKG_GDB_NATIVE_WANT_ED$PKG_GDB_M68K_BDM_WANT_ED" ]
then
    PKG_GDB_SERVER_WANT_ED=y
fi

# cross gdb to run on the build machine
if [ -n "$PKG_GDB_CROSS_WANT_ED" ]
then
    BD=cross-gdb
    rm -rf $BD
    mkdir -p $BD
    cd $BD
    export PATH=$UNSPOOF_PATH
    export ac_cv_header_stdc=yes bash_cv_have_mbstate_t=yes \
           ac_cv_header_nlist_h=no
    ../configure --prefix=%{_prefix} --target=$CFGHOST --mandir=%{_mandir}
    make
    cp gdb/gdb $TOP/bin/gdb
    cd -
fi

# from now build stuff to go into the rpm package
export PATH=$ORIG_PATH

# gdbserver to run on the target
if [ -n "$PKG_GDB_SERVER_WANT_ED" ]
then
    cd gdb/gdbserver
    sh ./configure --prefix=%{_prefix} --host=$CFGHOST --mandir=%{_mandir} --enable-threads --enable-shared
    make
    cd -
fi

# full gdb to run on the target
if [ -n "$PKG_GDB_NATIVE_WANT_ED" ]
then
    export CPP="$BUILDCPP"
    export CC_FOR_BUILD="$BUILDCC"
    export ac_cv_header_stdc=yes bash_cv_have_mbstate_t=yes \
           ac_cv_header_nlist_h=no
    export noconfigdirs=gdbserver
    CC=gcc AR=ar \
    ./configure --prefix=%{_prefix} --host=$OPT_CFGHOST --build=%{_build} --mandir=%{_mandir}
    make
    cd -
fi

# BDM capable gdb plus BDM utilities
if [ -n "$PKG_GDB_M68K_BDM_WANT_ED" ]
then
    BDM=m68k-bdm-1.3.0
    rm -rf ${BDM}-build
    mkdir -p ${BDM}-build || true
    cd ${BDM}-build
    export PATH=$UNSPOOF_PATH
    sh ../${BDM}/configure --prefix=%{_prefix} --enable-remote --enable-ioperm \
                           --enable-driver --enable-server --target=$CFGHOST
    make
    cp -a ../${BDM}/driver .
    cd driver/linux/
    make
    cd -
    cp test/bdm-chk $TOP/bin/m68k-linux-bdm-chk
    cp test/bdm-cpu32-chk $TOP/bin/m68k-linux-bdm-cpu32-chk
    cp utils/bdmflash $TOP/bin/m68k-linux-bdmflash
    cp utils/bdmmon $TOP/bin/m68k-linux-bdmmon
    cp utils/bdmreset $TOP/bin/m68k-linux-bdmreset
    cp driver/linux/m68k-bdm.ko $TOP/bin/
    cp driver/linux/m68k-bdm.o $TOP/bin/
    cd ..

    GDB=m68k-linux-gdb
    rm -rf $GDB
    mkdir -p $GDB
    cd $GDB
    export ac_cv_header_stdc=yes bash_cv_have_mbstate_t=yes \
           ac_cv_header_nlist_h=no
    ../configure --enable-gdbcli --enable-ioperm \
                 --target=m68k-bdm-linux --prefix=%{_prefix}
    make
    cp gdb/gdb $TOP/bin/m68k-linux-bdm-gdb
    cp gdb/gdbtui $TOP/bin/m68k-linux-bdm-gdbtui
    cd -
fi

%Install
if [ -z "$PKG_GDB_CROSS_WANT_ED$PKG_GDB_SERVER_WANT_ED$PKG_GDB_NATIVE_WANT_ED$PKG_GDB_M68K_BDM_WANT_ED" ]
then
    PKG_GDB_SERVER_WANT_ED=y
fi

rm -rf $RPM_BUILD_ROOT
if [ -n "$PKG_GDB_CROSS_WANT_ED" ]
then
    mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/share/info/.gdb_dummy_file
fi
if [ -n "$PKG_GDB_SERVER_WANT_ED" ]
then
    cd gdb/gdbserver
    make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
    cd -
fi
if [ -n "$PKG_GDB_NATIVE_WANT_ED" ]
then
    make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
    # remove standards.info which conflicts with autoconf
    rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/info/standards.info
fi
if [ -n "$PKG_GDB_M68K_BDM_WANT_ED" ]
then
    cp 5485.gdb $TOP/bin/
fi

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
