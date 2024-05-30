%define __os_install_post %{nil}
%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Base Libraries (from toolchain).
Name            : base_libs
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
X_DIR=`echo $TOOLCHAIN_PREFIX | sed -e 's,-$,,'`
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
set +e
# the second term is in there for CodeSorcery toolchains
for i in $SLIBS libc/$SLIBS
do
    if [ -d ${TC_PREFIX}/${X_DIR}/$i ]
    then
        cp -dp ${TC_PREFIX}/${X_DIR}/$i/*.so* ${RPM_BUILD_ROOT}/%{pfx}/lib/
    fi
done
set -e
if [ -z "$UCLIBC" ]
then
    # This takes care of crosstool versus CodeSourcery toolchains
    for P in ${TC_PREFIX}/${X_DIR} ${TC_PREFIX}/${X_DIR}/libc
    do
        if [ -f $P/bin/ldd ]
        then 
            cp -a $P/bin/ldd       ${RPM_BUILD_ROOT}/%{pfx}/usr/bin
            cp -a $P/sbin/ldconfig ${RPM_BUILD_ROOT}/%{pfx}/sbin
            break
        fi
    done
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
        # This takes care of crosstool versus CodeSourcery toolchains
        for P in ${TC_PREFIX}/${X_DIR}/include ${TC_PREFIX}/${X_DIR}/libc/usr/include
        do
            if [ -d $P ]
            then 
                cp -a $P ${RPM_BUILD_ROOT}/%{pfx}/usr
                break
            fi
        done
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
    # the second term is in there for CodeSorcery toolchains
    for i in $SLIBS libc/usr/$SLIBS
    do
        if [ -d ${TC_PREFIX}/${X_DIR}/$i ]
        then
            cp -dp ${TC_PREFIX}/${X_DIR}/$i/*.a ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/
        fi
    done
fi
cd ${RPM_BUILD_ROOT}/%{pfx}
# this is necessary to avoid annoying warnings from ldd about no execute
# permissions for some malformed uClibc toolchains
find ${RPM_BUILD_ROOT}/%{pfx}/lib     | xargs chmod 755
find ${RPM_BUILD_ROOT}/%{pfx}/usr/lib | xargs chmod 755
#find lib/ -type f -o -type d   | xargs chmod 755 
#find usr/lib/ -type f -o -type d | xargs chmod 755 
stripall

# relocate all symlink .so (linker files) to usr/lib
find . -name \*.so | perl -n -e '
   chomp;
   next unless -l $_ ;
   $ltgt = readlink();
   $snam = substr($_, rindex($_, "/") + 1);
   print "Moving symlink $_ ($ltgt) to usr/lib/$snam\n";
   unlink $_;
   symlink("../../lib/$ltgt", "usr/lib/$snam") 
                         or die "symlink(../../lib/$ltgt, usr/lib/$snam) : $!\n";
' 

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
