%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir tslib

Summary         : Abstraction layer for touchscreen panel events 
Name            : tslib
Version         : 1.7.1
Release         : 0
License         : LGPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : System Environment/Libraries
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n %{buildsubdir}
sed -i 's,35,100,'  tests/ts_calibrate.c

%Build
#sed -i s/AS_HELP_STRING/AC_HELP_STRING/ configure.ac
./autogen.sh
chmod +x ./configure
export ac_cv_func_malloc_0_nonnull=yes
export shrext='.so'
./configure CC=gcc --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --enable-shared=yes --enable-debug=no
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
./autogen-clean.sh
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ts/*.la
#strip 
strip ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/*
strip ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/*
strip ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/ts/*
mkdir -p $RPM_BUILD_ROOT%{pfx}/etc
mkdir -p $RPM_BUILD_ROOT%{pfx}/home/sagem

rm -f $RPM_BUILD_ROOT%{pfx}/etc/pointercal $RPM_BUILD_ROOT%{pfx}/etc/pointercal.backup
cat << EOF > $RPM_BUILD_ROOT%{pfx}/etc/pointercal.backup
54064 40 -1855672 -326 34725 -2464760 65536
EOF
cp $RPM_BUILD_ROOT%{pfx}/etc/pointercal.backup $RPM_BUILD_ROOT%{pfx}/home/sagem/pointercal
ln -s ../home/sagem/pointercal $RPM_BUILD_ROOT%{pfx}/etc/pointercal

cat <<EOF > $RPM_BUILD_ROOT/%{pfx}/usr/bin/do_calibration.sh
#! /bin/sh

export TSLIB_FBDEVICE=/dev/fb0
export TSLIB_CONSOLEDEVICE=none
export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_IMX31_DEFAULTVALUES=true
ts_calibrate
echo "CALIBRATION_DONE()" >> /var/msglib/poperammi

# save calibration into the factory zone
/etc/rc.d/init.d/calibration setfactory
EOF

cat <<EOF > $RPM_BUILD_ROOT/%{pfx}/usr/bin/test_touchscreen.sh
#! /bin/sh

export TSLIB_FBDEVICE=/dev/fb0
export TSLIB_CONSOLEDEVICE=none
export TSLIB_TSDEVICE=/dev/input/event1
export TSLIB_IMX31_DEFAULTVALUES=true
ts_calibrate -2
EOF
chmod a+x $RPM_BUILD_ROOT/%{pfx}/usr/bin/do_calibration.sh
chmod a+x $RPM_BUILD_ROOT/%{pfx}/usr/bin/test_touchscreen.sh

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
