%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : The GNU patch command, for modifying/upgrading files
Name            : patch
Version         : 2.5.4
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Tools
Source          : patch-2.5.4.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=${_build} --mandir='${prefix}/share/man'
make

%Install
rm -rf $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT/%{pfx}/%{_prefix} install

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


