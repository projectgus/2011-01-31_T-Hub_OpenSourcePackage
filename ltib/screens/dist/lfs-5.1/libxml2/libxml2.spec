%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Libraries, includes, etc. to develop XML/HTML applications
Name            : libxml2
Version         : 2.6.28
Release         : 1
Vendor          : Freescale
Packager        : Jason Jin/Stuart Hughes/Kurt Mahan
Group           : Development/Libraries
Source          : %{name}-%{version}.tar.gz
License         : MIT
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --with-history --without-python
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
find $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/ -name "*.la" | xargs rm -f
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/bin/xmlcatalog
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/bin/xmllint
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/usr/lib/*.so*
%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/usr/include/*
%{pfx}/usr/bin/*
%{pfx}/usr/lib/*
