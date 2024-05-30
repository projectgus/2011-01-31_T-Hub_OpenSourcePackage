%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Samba is useful for creating and connecting to Windows shares using the SMB protocol
Name            : samba
Version         : 3.0.2a
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Joe D'Abbraccio
Group           : System Environment/Daemons
Source          : %{name}-%{version}.tar.gz
Patch1          : samba-3.0.2-configure-cross-2.patch
Patch2          : samba-3.0.2-gcc4-fix.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 
%patch1 -p1
%patch2 -p1

%Build
cd source
CPPFLAGS="-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE" \
samba_cv_HAVE_EXPLICIT_LARGEFILE_SUPPORT=yes \
samba_cv_have_longlong=yes \
samba_cv_SIZEOF_OFF_T=yes \
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --disable-cups
make -j1

%Install
rm -rf $RPM_BUILD_ROOT
cd source
make -j1 installbin DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
