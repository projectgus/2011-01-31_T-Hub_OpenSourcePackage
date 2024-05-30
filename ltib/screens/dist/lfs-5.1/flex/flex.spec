%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : fast lexical analyzer generator
Name            : flex
Version         : 2.5.4
Release         : 1
License         : BSD
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Tools
Source          : flex-2.5.4a.tar.gz
Patch1          : flex-2.5.4-mandir.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
%setup
%patch1 -p1

%Build
./configure --prefix=%{_prefix}
make

%Install
rm -rf $RPM_BUILD_ROOT
make -j1 install prefix=$RPM_BUILD_ROOT/%{pfx}/%{_prefix}
ln -s flex $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin/lex

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


