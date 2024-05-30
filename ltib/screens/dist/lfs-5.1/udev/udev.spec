%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Userspace device files
Name            : udev
Version         : 056
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Alan Tull
Group           : admin
Source          : %{name}-%{version}.tar.bz2
Patch0		: udev_0.056-1.diff
Patch1          : udev-056-initscript.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1
patch -p1 < debian/patches/enable_after_udev
patch -p1 < debian/patches/no_strip
%patch1 -p1

sed -ri 's,PATH="/sbin:/bin",PATH="/sbin:/bin:/usr/sbin:/usr/bin",' debian/udev.init
sed -ri 's,    warn_if_interactive,#    warn_if_interactive,' debian/udev.init

# Add a couple of devices for irda stack
echo "M ircomm0       c 161 0"  >> extra/links.conf
echo "M irnet         c 10 187" >> extra/links.conf

%Build
make udevdir=/dev V=true

%Install
rm -rf $RPM_BUILD_ROOT
make install udevdir=/dir DESTDIR=$RPM_BUILD_ROOT/%{pfx} V=true

install -d $RPM_BUILD_ROOT/%{pfx}/sbin
install -d $RPM_BUILD_ROOT/%{pfx}/etc/rc.d/init.d
install -d $RPM_BUILD_ROOT/%{pfx}/etc/udev/scripts

install debian/udev.init $RPM_BUILD_ROOT/%{pfx}/etc/rc.d/init.d/udev

install -m 755 extra/scsi-devfs.sh extras/cdsymlinks.sh extras/dvb.sh \
	       extras/ide-devfs.sh extras/inputdev.sh extra/ide-model.sh \
	       extra/raid-devfs.sh \
	       $RPM_BUILD_ROOT/%{pfx}/etc/udev/scripts

install -m 644 extra/*.rules extra/links.conf extras/cdsymlinks.conf \
	       $RPM_BUILD_ROOT/%{pfx}/etc/udev/

sed -e 's/^#\([^ ]\)/\1/' < extra/compat.rules > \
                $RPM_BUILD_ROOT/%{pfx}/etc/udev/compat-full.rules

install -m 644 $RPM_BUILD_ROOT/%{pfx}/etc/udev/compat.rules \
	   extra/devfs.rules \
	   $RPM_BUILD_ROOT/%{pfx}/etc/udev/rules.d/

rm $RPM_BUILD_ROOT/%{pfx}/etc/hotplug.d/default/10-udev.hotplug
ln -sfn /sbin/udevsend $RPM_BUILD_ROOT/%{pfx}/etc/hotplug.d/default/10-udev.hotplug

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
