%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : zlib compression utilities and libraries
Name            : zlib
Version         : 1.1.4
Release         : 2
License         : zlib (Distributable)
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Libraries
Source          : zlib-1.1.4.tar.gz
Patch0          : zlib-1.1.4-arflags.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n zlib-%{version}
%patch0 -p1

%Build
./configure --prefix=%{_prefix} --shared
mv Makefile Makefile.shared
#./configure --prefix=%{_prefix} SAGEM remove static library
#mv Makefile Makefile.static SAGEM remove static library
make -f Makefile.shared
#make -f Makefile.static SAGEM remove static library

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}
make -f Makefile.shared install prefix=${RPM_BUILD_ROOT}/%{pfx}/%{_prefix}
#make -f Makefile.static install prefix=${RPM_BUILD_ROOT}/%{pfx}/%{_prefix} SAGEM remove static library
#strip 
${TOOLCHAIN_PREFIX}strip ${RPM_BUILD_ROOT}/%{pfx}/usr/lib/*

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


