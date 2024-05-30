%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Utilities for ALSA (Advanced Linux Sound Architecture)
Name            : alsa-utils
Version         : 1.0.11rc2
# 1.0.11rc2 is compatible with linux 2.6.16 kernels
#   (see linux/include/sound/version.h)
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : Applications/System
Source          : alsa-utils-%{version}.tar.bz2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la
rm -rf ${RPM_BUILD_ROOT}/%{pfx}/usr/share/sounds
#strip 
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/usr/bin/*
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/usr/sbin/alsactl

mkdir -p $RPM_BUILD_ROOT/%{pfx}/etc
cat << _EOF > $RPM_BUILD_ROOT/%{pfx}/etc/asound.conf
pcm.dmixer {
    type dmix
    ipc_key 1024		# must be unique!
#   ipc_key_add_uid false	# let multiple users share
#   ipc_perm 0666		# IPC permissions for multi-user sharing (octal, default 0600)
    slave {
        pcm "hw:0,0"		# you cannot use a "plug" device here.
        period_time 0
        period_size 1024	# must be power of 2
        buffer_size 8192	# ??
        #format "S32_LE"
        #periods 128		# ??
        rate 44100		# let's choose bitrate
    }
    # bindings are cool. This says, that only the first
    # two channels are to be used by dmix, which is
    # enough for (most) oss apps and also lets
    # multichannel chios work much faster:
    bindings {
        0 0 # from 0 => to 0
        1 1 # from 1 => to 1
    }
}
pcm.!default {
    type plug
    slave.pcm "dmixer"
}
pcm.dsp0 {
    type plug
    slave.pcm "dmixer" # use our new PCM here
}
# mixer0 like above
ctl.mixer0 {
    type hw
    card 0
}
_EOF
chmod 644 $RPM_BUILD_ROOT/%{pfx}/etc/asound.conf

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
