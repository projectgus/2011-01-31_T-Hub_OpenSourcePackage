%define __os_install_post %{nil}
%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Base Libraries (from toolchain).
Name            : base_libs_mv
Version         : 1.0
Release         : 1
License         : LGPL
Vendor          : Freescale Inc.
Packager        : Steve Papacharalambous/Stuart Hughes
Group           : System Environment/Libraries
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep

%Build

%Install
rm -rf $RPM_BUILD_ROOT
X_DIR=target 
#`echo $TOOLCHAIN_PREFIX | sed -e 's,-$,,'`
if [ "$TOOLCHAIN_TYPE" = "64" ]
then
    SLIBS=lib64
else
    SLIBS=lib
fi
TC_PREFIX="`which ${TOOLCHAIN_PREFIX}gcc | perl -p -e 's,/bin/'${TOOLCHAIN_PREFIX}'gcc,,'`"
for i in etc usr/bin sbin lib usr/lib usr/share
do
    mkdir -p ${RPM_BUILD_ROOT}/%{pfx}/$i
done
( cp -dp ${TC_PREFIX}/${X_DIR}/$SLIBS/*.so* ${RPM_BUILD_ROOT}/%{pfx}/lib/ ; exit 0)
( cp -dp ${TC_PREFIX}/${X_DIR}/usr/lib/libstdc++.so* ${RPM_BUILD_ROOT}/%{pfx}/lib/ ; exit 0)
if [ -z "$UCLIBC" ]
then
    cp -a ${TC_PREFIX}/${X_DIR}/usr/bin/ldd ${RPM_BUILD_ROOT}/%{pfx}/usr/bin
    cp -a ${TC_PREFIX}/${X_DIR}/sbin/ldconfig ${RPM_BUILD_ROOT}/%{pfx}/sbin
fi

if [ -n "$PKG_LIBC_WANT_LOCALES" ]
then
    if [ -n "$UCLIBC" ]
    then
        cp -a ${TC_PREFIX}/usr/share/locale ${RPM_BUILD_ROOT}/%{pfx}/usr/share
    else
        for i in tzselect locale localedef
        do
            cp -a ${TC_PREFIX}/${X_DIR}/bin/$i ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/
        done
        cp -a ${TC_PREFIX}/${X_DIR}/share/locale ${RPM_BUILD_ROOT}/%{pfx}/usr/share
        cp -a ${TC_PREFIX}/${X_DIR}/$SLIBS/gconv ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/
    fi
fi

if [ -n "${PKG_LIBC_WANT_HEADERS}" ]
then
    if [ -n "$UCLIBC" ]
    then
        cp -a ${TC_PREFIX}/include ${RPM_BUILD_ROOT}/%{pfx}/usr
    else
        cp -a ${TC_PREFIX}/${X_DIR}/include ${RPM_BUILD_ROOT}/%{pfx}/usr
    fi
fi

# remove absolute paths from text search files (if they exist)
perl -w -e '
    @ARGV = grep { `file $_` =~ m,ASCII C program text, } @ARGV;
    exit(0) unless @ARGV;
    $^I = ".bak";
    while(<>) {
        s,[\S/]+/,,g if m,^GROUP,;
        print;
    }
    ' ${RPM_BUILD_ROOT}/%{pfx}/lib/libc.so \
      ${RPM_BUILD_ROOT}/%{pfx}/lib/libpthread.so \
      ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libc.so \
      ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/libpthread.so

if [ -n "$PKG_LIBC_WANT_STATIC_LIBS" ]
then 
    cp -dp ${TC_PREFIX}/${X_DIR}/$SLIBS/*.a ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/
fi
cd ${RPM_BUILD_ROOT}/%{pfx}
stripall


%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
