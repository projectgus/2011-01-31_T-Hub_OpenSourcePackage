%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : qtopia palmtop environment
Name            : qtopia-free-src
Version         : 2.2.0
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : John Rigby
Group           : System Environment/Graphics
Source          : %{name}-%{version}.tar.gz
Patch1		: qtopia-free-2.2.0-mpc5200-0001-19Jun2006-ppc-qconfig.patch
Patch2		: qtopia-free-2.2.0-mpc5200-0002-19Jun2006-ppc-libavcodec-bigendian.patch
Patch3		: qtopia-free-2.2.0-mpc5200-0003-19Jun2006-qconfig-cursor-mouse.patch
Patch4		: qtopia-free-2.2.0-mpc5200-0004-19Jun2006-rgb555-swapped-video.patch
Patch5		: qtopia-free-2.2.0-mpc5200-0005-19Jun2006-audio-no-getospace.patch
Patch6		: qtopia-free-2.2.0-mpc5200-0006-19Jun2006-nativeendian-audio.patch
Patch7		: qtopia-free-2.2.0-mpc5200-0007-19Jun2006-media5200-touchscreen.patch
Patch8		: qtopia-free-2.2.0-mpc5200-0008-19Jun2006-runtime-hidecursor.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n qtopia-free-2.2.0
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1
cp -r tmake/lib/qws/linux-arm-g++ tmake/lib/qws/linux-powerpc-g++
rm -rf tmake/lib/qws/linux-powerpc-linux-g++/tmake.conf
cat tmake/lib/qws/linux-arm-g++/tmake.conf | sed -e "s/arm-linux/powerpc-603e-linux/" > tmake/lib/qws/linux-powerpc-g++/tmake.conf
cp -r qtopia/mkspecs/qws/linux-arm-g++ qtopia/mkspecs/qws/linux-powerpc-g++
rm -rf qtopia/mkspecs/qws/linux-powerpc-g++/qmake.conf 
cat qtopia/mkspecs/qws/linux-arm-g++/qmake.conf | sed -e "s/arm-linux/powerpc-603e-linux/" > qtopia/mkspecs/qws/linux-powerpc-g++/qmake.conf
# force the calibration apps on for touchscreen
sed < qtopia/src/libraries/qtopia/custom-linux-generic-g++.h -e 's/.*#.*QPE_NEED_CALIBRATION/#define QPE_NEED_CALIBRATION/' > qtopia/src/libraries/qtopia/custom-linux-powerpc-g++.h
cp qtopia/src/libraries/qtopia/custom-linux-generic-g++.cpp qtopia/src/libraries/qtopia/custom-linux-powerpc-g++.cpp


%Build
export PATH=$UNSPOOF_PATH
echo yes | ./configure \
  -qte    "-platform linux-x86-g++ -xplatform linux-powerpc-g++ \
    -depths 16,32 -system-jpeg -no-opengl -no-xft -no-sm -no-qvfb -no-vnc \
    -rgb555 -swapbytes_video \
    -no-g++-exceptions -no-xft -embedded -qconfig qpe \
    -L $DEV_IMAGE/usr/lib \
    -I $DEV_IMAGE/usr/include" \
  -qpe    "-platform linux-g++ -xplatform linux-powerpc-g++ \
    -arch powerpc -no-qvfb -with-libffmpeg -qconfig qpe \
    -rgb555 -swapbytes_video \
    -L $DEV_IMAGE/usr/lib \
    -I $DEV_IMAGE/usr/include \
    -l jpeg -l uuid"
./qtopia/scripts/buildQtopiaTools -check
make

%Install
export PATH=$UNSPOOF_PATH
echo creating qtopia install image
make install > /dev/null
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/%{pfx}
cp -a qtopia/image/opt $RPM_BUILD_ROOT/%{pfx}
install -d $RPM_BUILD_ROOT/%{pfx}/etc/rc.d
cat > $RPM_BUILD_ROOT/%{pfx}/etc/rc.d/rc.qpe << EOF
#!/bin/sh
#
# Copyright 2006, Freescale Semiconductor Inc.
#
# Set QPE_USE_TOUCHSCREEN to "y" to use touchscreen 
# otherwise mouse will be used
#
export HOME=/root
export QPE_USE_TOUCHSCREEN="$PKG_QTOPIA_WANT_TOUCHSCREEN"
# make sure usb input and touchscreen node exist
mkdir -p /dev/input
test -c /dev/input/mice || mknod /dev/input/mice c 13 63
test -c /dev/ts || mknod /dev/ts c 11 0
export QTDIR=/opt/Qtopia
export QPEDIR=/opt/Qtopia
export LD_LIBRARY_PATH=/opt/Qtopia/lib
export PATH=/opt/Qtopia/bin:\$PATH
export QWS_SIZE="1024x768"
if [ _\$QPE_USE_TOUCHSCREEN = "_y" ]
then
# changing the :/dev/ts part will have no effect as it 
# is hardcoded in the source
export QWS_MOUSE_PROTO="TPanel:/dev/ts"
# hide cursor when using touchscreen
export QWS_HIDE_CURSOR="Yes"
grep touchscreen /proc/devices > /dev/null || insmod mpc5200_touch.ko
# remove pointercal if it is empty
[ -f /etc/pointercal -a ! -s /etc/pointercal ] && rm -f /etc/pointercal
else
# no QWS_KEYBOARD config it just defaults to /dev/tty0
# which means virtual console must be on
export QWS_MOUSE_PROTO="USB:/dev/input/mice"
# overide pointer calibration when using mouse
touch /etc/pointercal 
fi
cd /opt/Qtopia/bin
#./qpe > /dev/null 2>&1 &
./qpe &
EOF
chmod 744 $RPM_BUILD_ROOT/%{pfx}/etc/rc.d/rc.qpe
echo '1' > $RPM_BUILD_ROOT/%{pfx}/etc/firstuse
# setup Storage.conf
install -d $RPM_BUILD_ROOT/%{pfx}/root/Settings
cat > $RPM_BUILD_ROOT/%{pfx}/root/Settings/Storage.conf << EOF
[rootfs]
Name = Internal Storage
Removeable = 0
[/dev/sda1]
Name = USB Flash Drive
Removable = 1
EOF

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


