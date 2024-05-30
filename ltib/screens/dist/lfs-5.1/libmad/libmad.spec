%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A library for mad mp3 decoding
Name            : libmad
Version         : 0.15.1b
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Ross Wille
Group           : System Environment/Libraries
Source          : libmad-0.15.1b.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup -n libmad-0.15.1b

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --mandir=%{_mandir}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/*.la

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*
