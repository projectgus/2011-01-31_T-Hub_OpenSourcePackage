%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : LMbench performance analysis tools
Name            : lmbench-3.0
Version         : a4
Release         : 1
License         : GPL + restrictions
Vendor          : Freescale
Packager        : John Faith
Group           : Testing
Source          : lmbench-3.0-a4.tgz
#Patch0          : lmbench-fsl-3.0-a4.patch
Patch1          : lmbench-3.0-a4-cc.patch
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
URL             : http://lmbench.sourceforge.net/whatis_lmbench.html

%Description
%{summary}

%Prep
%setup
#%patch0 -p1
%patch1 -p1

%Build
make

%Install
instDir=$RPM_BUILD_ROOT/%{pfx}/opt/lmbench
rm -rf $instDir
mkdir -p $instDir
cd src
make install BASE=$instDir
cd -

# Also install the lmbench scripts
cp -a scripts results $instDir

# Install webpages for http tests
mkdir -p $instDir/src
cp src/webpage-lm.tar $instDir/src

# Force os script to just say "linux", and move
# binaries to bin/linux/.
cd $RPM_BUILD_ROOT/%{pfx}/opt/lmbench
cp scripts/lmbench bin
mv bin linux; mkdir bin; mv linux bin
cd scripts
chmod 755 os
cat >os <<TEXT
#!/bin/sh
echo linux
TEXT


%Clean
rm -rf $RPM_BUILD_ROOT

%Files
%defattr(-,root,root)
%{pfx}/*

