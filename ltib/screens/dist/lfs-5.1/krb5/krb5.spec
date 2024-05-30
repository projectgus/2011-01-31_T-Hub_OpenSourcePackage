%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : Development files needed to compile Kerberos 5 programs
Name            : krb5
Version         : 1.3.4
Release         : 1
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Libraries
Source          : %{name}-%{version}.tar.gz
License         : MIT (freely distributable)
BuildRoot       : %{_tmppath}/%{name}

%Description
%{summary}

%Prep
%setup 

%Build
cd src
./configure --prefix=%{_prefix} --enable-shared  --without-tcl
make

%Install
rm -rf $RPM_BUILD_ROOT
cd src
make install DESTDIR=$RPM_BUILD_ROOT/%{pfx}
rm -rf $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/man
for i in libcom_err.so libcom_err.a
do
    rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/$i
done
for i in ftp rcp rlogin rsh telnet compile_et
do
    rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/bin/$i
done
for i in et_c.awk et_h.awk
do 
    rm -f $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/share/et/$i
done

%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*
