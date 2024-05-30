%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Tools for managing kernel packet filtering capabilities
Name            : iptables
Version         : 1.3.8
Release         : 4
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes, Emil Medve
Group           : System Environment/Base
URL             : http://www.netfilter.org/projects/iptables
Source          : %{name}-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

iptables is the userspace command line program used to configure the Linux 2.4.x and 2.6.x IPv4
packet filtering ruleset. It is targeted towards system administrators

Since Network Address Translation is also configured from the packet filter ruleset, iptables is
used for this, too

The iptables package also includes ip6tables. ip6tables is used for configuring the IPv6 packet
filter

iptables requires a kernel that features the ip_tables packet filter. This includes all 2.4.x and
2.6.x kernel releases

Main Features
        * listing the contents of the packet filter ruleset
        * adding/removing/modifying rules in the packet filter ruleset
        * listing/zeroing per-rule counters of the packet filter ruleset

%Prep
%setup

%Build
make KERNEL_DIR=$DEV_IMAGE/%{_prefix}/src/linux PREFIX=%{_prefix}

%Install
rm -rf $RPM_BUILD_ROOT
make install KERNEL_DIR=$DEV_IMAGE/%{_prefix}/src/linux DESTDIR=$RPM_BUILD_ROOT/%{pfx} PREFIX=%{_prefix}
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/lib/iptables/*.so
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/sbin/*
mkdir -p $RPM_BUILD_ROOT/%{pfx}/etc
#create script to flush iptables ruleset--------------------------------------------
cat <<EOF > $RPM_BUILD_ROOT/%{pfx}/etc/iptables_flush.sh
#!/bin/sh

iptables -F
iptables -X
iptables -P INPUT   ACCEPT
iptables -P OUTPUT  ACCEPT
iptables -P FORWARD ACCEPT

EOF
chmod 755 $RPM_BUILD_ROOT/%{pfx}/etc/iptables_flush.sh

#create script to start iptables ruleset--------------------------------------------
cat <<EOF > $RPM_BUILD_ROOT/%{pfx}/etc/iptables_start.sh
#!/bin/sh

#external interface
#IF=eth1

#flush
iptables -F
iptables -X

#logs
iptables -N LOG_DROP
iptables -A LOG_DROP -m limit --limit 1/s -j LOG --log-level 7 --log-prefix '[DROPPED PACKET] : '
iptables -A LOG_DROP -j DROP

#let's drop everything
iptables -P INPUT   ACCEPT
iptables -P OUTPUT  ACCEPT
iptables -P FORWARD ACCEPT

#local connections and pings are allowed
iptables -A INPUT  -i lo -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT
iptables -A INPUT -p icmp -j ACCEPT
iptables -A OUTPUT -p icmp -j ACCEPT

#outgoing connections OK
iptables -A OUTPUT -m state --state NEW,ESTABLISHED,RELATED -j ACCEPT
iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

#incoming connections filtering
#dropbear
#iptables -A INPUT -m state --state NEW,ESTABLISHED,RELATED -p tcp --dport 22 -j ACCEPT
#DLNA file servers
iptables -A INPUT -m state --state NEW,ESTABLISHED,RELATED -p tcp --dport 9051 -j ACCEPT #usb
iptables -A INPUT -m state --state NEW,ESTABLISHED,RELATED -p tcp --dport 9150 -j ACCEPT #usb
iptables -A INPUT -m state --state NEW,ESTABLISHED,RELATED -p tcp --dport 9001 -j ACCEPT #mmc
iptables -A INPUT -m state --state NEW,ESTABLISHED,RELATED -p tcp --dport 9100 -j ACCEPT #mmc
#DLNA control point
iptables -A INPUT -m state --state NEW,ESTABLISHED,RELATED -p tcp --dport 9000 -j ACCEPT
#DLNA multicast
iptables -A INPUT -p udp -d 239.255.255.250 --dport 1900 -j ACCEPT
#DLNA SSDP, reserved source ports, shouldn't accept incoming connections on app side, but authorize anyway
iptables -A INPUT -p udp --dport 9080 -j ACCEPT
iptables -A INPUT -p udp --dport 9081 -j ACCEPT
iptables -A INPUT -p udp --dport 1030 -j ACCEPT
iptables -A INPUT -p udp --dport 1031 -j ACCEPT


#send those dropped packets to log
iptables -A FORWARD -j LOG_DROP
iptables -A INPUT -j LOG_DROP
iptables -A OUTPUT -j LOG_DROP

EOF
chmod 755 $RPM_BUILD_ROOT/%{pfx}/etc/iptables_start.sh

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
