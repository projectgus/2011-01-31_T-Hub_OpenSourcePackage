%define pfx /opt/freescale/rootfs/%{_target_cpu}
%define buildsubdir wireless-tools

Summary         : Wireless LAN configuration tools
Name            : wireless-tools
Version         : 29
Release         : 1
License         : GPL
Vendor          : Hewlett-Packard
Packager        : Olivier Le Roy
Group           : System Environment/Base
Source          : %{name}-%{version}.tar.gz

BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}

%Description
%{summary}

%Prep
rm -rf %{buildsubdir}
%setup -n %{buildsubdir}

%Build
make

%Install
rm -rf $RPM_BUILD_ROOT
make install PREFIX=$RPM_BUILD_ROOT/%{pfx} INSTALL_INC=$RPM_BUILD_ROOT/%{pfx}/%{_prefix}/include/ INSTALL_MAN=$RPM_BUILD_ROOT/%{pfx}/%{_prefix}/share/man/
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/lib/*.so*
${TOOLCHAIN_PREFIX}strip $RPM_BUILD_ROOT/%{pfx}/sbin/*

%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*

%changelog
* Thu May 15 2008 Le Roy Olivier <olivier.leroy@sagem.com>
- wireless-tools version 28 -> version 29 fixes bugs.
* Mon Feb 26 2007 Isenegger Laurent <laurent.isenegger@sagem.com>
- Initial build.