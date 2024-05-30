%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Dropbear SSH server and client (embedded)
Name            : dropbear
Version         : 0.45
Release         : 1
License         : MIT
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Applications/Internet
Source          : %{name}-%{version}.tar.gz
Patch1          : dropbear-0.45-scp_warnings.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch1 -p1

%Build
if [ "$PKG_DROPBEAR_WANT_URANDOM_DEV" = "y" ]
then
    perl -pi -e 's,^#define DROPBEAR_RANDOM_DEV.*,#define DROPBEAR_RANDOM_DEV "/dev/urandom",; ' options.h
fi
if [ "$PKG_DROPBEAR_WANT_NO_REV_DNS" = "y" ]
then
    perl -pi -e 's,^(#define DO_HOST_LOOKUP),//\1,; ' options.h
fi
if [ "$PKG_DROPBEAR_WANT_NO_X11FWD" = "y" ]
then
    perl -pi -e 's,^(#define ENABLE_X11FWD),//\1,; ' options.h
fi

LD=gcc ./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make PROGRAMS="dropbear dbclient dropbearkey dropbearconvert scp" MULTI=1 SCPPROGRESS=1

%Install
rm -rf $RPM_BUILD_ROOT
for i in bin sbin
do
    mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/$i
done
mkdir -p $RPM_BUILD_ROOT/%{pfx}/etc/dropbear
cp dropbearmulti $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/sbin/
ln -s %{_prefix}/sbin/dropbearmulti $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/sbin/dropbear
for i in dbclient dropbearconvert dropbearkey scp
do
    ln -s %{_prefix}/sbin/dropbearmulti $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin/$i
done

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
