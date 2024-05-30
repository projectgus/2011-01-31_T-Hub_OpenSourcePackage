%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir udev

Summary         : Userspace device files
Name            : udev
Version         : 091_1.1
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : admin
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n %{buildsubdir}

%Build
make udevdir=/dev V=true

%Install
rm -rf $RPM_BUILD_ROOT
make install udevdir=/dir DESTDIR=$RPM_BUILD_ROOT/%{pfx} V=true

install -d $RPM_BUILD_ROOT/%{pfx}/sbin
install -d $RPM_BUILD_ROOT/%{pfx}/etc/rc.d/init.d
install -d $RPM_BUILD_ROOT/%{pfx}/etc/udev/scripts
install -d $RPM_BUILD_ROOT/%{pfx}/etc/hotplug.d/default

install extra/udev.startup $RPM_BUILD_ROOT/%{pfx}/etc/rc.d/init.d/udev
install -m 755 udevsynthesize udevsend $RPM_BUILD_ROOT/%{pfx}/sbin

install -m 755 extra/*.sh extras/*.sh \
	       $RPM_BUILD_ROOT/%{pfx}/etc/udev/scripts

install -m 644 extra/links.conf \
           $RPM_BUILD_ROOT/%{pfx}/etc/udev/

sed -e 's/^#\([^ ]\)/\1/' < extra/compat.rules > \
           $RPM_BUILD_ROOT/%{pfx}/etc/udev/compat-full.rules

install -m 644 extra/10-mx31.rules \
	   $RPM_BUILD_ROOT/%{pfx}/etc/udev/rules.d

install -m 644 extra/udev.rules \
	   $RPM_BUILD_ROOT/%{pfx}/etc/udev/rules.d/50-udev.rules

install -m 644 extra/compat.rules \
	   $RPM_BUILD_ROOT/%{pfx}/etc/udev/rules.d/60-compat.rules

install -m 644 extra/devfs.rules \
	   $RPM_BUILD_ROOT/%{pfx}/etc/udev/rules.d/70-devfs.rules

ln -sfn /sbin/udevsend $RPM_BUILD_ROOT/%{pfx}/etc/hotplug.d/default/10-udev.hotplug
#strip 
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/sbin/*
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/*
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/usr/sbin/*

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
