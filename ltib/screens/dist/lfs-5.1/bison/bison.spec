%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : A GNU general-purpose parser generator
Name            : bison
Version         : 1.875
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Tools
Source          : bison-1.875.tar.bz2
Patch0          : bison-1.875-attribute.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch0 -p1

%Build
./configure --prefix=%{_prefix} --host=$CFGHOST --build=%{_build} --mandir=%{_mandir}
make

%Install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


