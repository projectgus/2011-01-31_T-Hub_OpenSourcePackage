%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : The Device Tree Compiler for Flat Device Trees
Name            : dtc
Version         : 07jul06
Release         : 1
License         : GPL
Vendor          : Freescale
Packager        : Ebony Zhu
Group           : Development/Tools
Source          : %{name}-%{version}.tar.gz
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup 

%Build
make

%Install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin
cp -a dtc $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


