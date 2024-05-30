%define base %(echo %{_prefix} | sed -e s,/usr.*$,,)
%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Secure Sockets Layer toolkit
Name            : openssl
Version         : 0.9.8e
Release         : 1
License         : BSD style
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
case $ENDIAN in
    big)
        XTRA_OPTS="-DB_ENDIAN"
        ;;
    little)
        XTRA_OPTS="-DL_ENDIAN"
        ;;
    *)
        echo "Please set the ENDIAN environment variable to big|little"
        ;;
esac
case "$LINTARCH" in
    arm|m68k*)
       OSSL_ARCH="linux-generic32"
       ;;
    *)
       OSSL_ARCH="linux-$LINTARCH"
       ;;
esac
./Configure $OSSL_ARCH --prefix=%{_prefix} shared no-asm $XTRA_OPTS
make -j1

%Install
rm -rf $RPM_BUILD_ROOT
for i in lib %{_prefix}/lib %{_prefix}/sbin %{_prefix}/include usr/lib/pkgconfig
do
    mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{base}/$i
done
VER="`perl -e '$_  = shift; chop; print' %{version}`"
for i in libcrypto.so libssl.so
do 
    cp -a $i.$VER $RPM_BUILD_ROOT/%{pfx}/%{base}/lib/
    ln -s ../../lib/$i.$VER $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/$i
done
cp openssl.pc $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig
cp libcrypto.pc $RPM_BUILD_ROOT/%{pfx}/usr/lib/pkgconfig
cp -a apps/openssl $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/sbin
cp -Lr include/openssl $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/include

#strip 
STRIP=${TOOLCHAIN_PREFIX}strip
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/lib/*
$STRIP ${RPM_BUILD_ROOT}/%{pfx}/usr/sbin/*

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
