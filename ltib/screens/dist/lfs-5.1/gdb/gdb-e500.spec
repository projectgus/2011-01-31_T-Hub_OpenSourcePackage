%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define pkg_name gdb-e500

Summary         : Gdb - GNU Source level debugger for C, C++
Name            : gdb
Version         : 6.2
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Debuggers
Source          : gdb-jimb-gdb_6_2-e500-branch-20050510.tar.bz2
Patch0          : gdb-6.0-tcsetpgrp.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

Notes:

e500 version from jimb-gdb_6_2-e500-branch
requires: libtermcap and ncurses


%Prep
%setup -n %{pkg_name}-%{version}
%patch0 -p1

%Build
ORIG_PATH=$PATH
# This is the optimised host type.  Needed to prevent infering a native build
OPT_CFGHOST=`echo ${TOOLCHAIN_PREFIX} | perl -n -e 's,-$,,;print'`

# do something (the least costly)
if [ -z "$PKG_GDB_CROSS_WANT_ED$PKG_GDB_SERVER_WANT_ED$PKG_GDB_NATIVE_WANT_ED" ]
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
    ./configure --prefix=%{_prefix} --host=$CFGHOST --mandir=%{_mandir}
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
    CC=gcc AR=ar \
    ./configure --prefix=%{_prefix} --host=$OPT_CFGHOST --build=%{_build} --mandir=%{_mandir}
    make
fi

%Install
if [ -z "$PKG_GDB_CROSS_WANT_ED$PKG_GDB_SERVER_WANT_ED$PKG_GDB_NATIVE_WANT_ED" ]
then
    PKG_GDB_SERVER_WANT_ED=y
fi

rm -rf $RPM_BUILD_ROOT
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

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


