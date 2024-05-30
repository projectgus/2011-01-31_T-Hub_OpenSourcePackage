%define pfx /opt/freescale/rootfs/%{_target_cpu}

Summary         : The Perl programming language
Name            : perl
Version         : 5.8.8
Release         : 1
License         : Artistic or GPL
Vendor          : Freescale
Packager        : Stuart Hughes
Group           : Development/Languages
Source          : perl-5.8.8.tar.gz
Source1         : perl-5.8.0-Glob.pm.simple-2
BuildRoot       : %{_tmppath}/%{name}
Prefix          : %{pfx}
# perl auto module dependency is pretty broken in rpm, so we have
# to turn off all dependency checks
Autoreqprov     : no


%Description
%{summary}

%Prep
%setup 

%Build
#
# build a version for the machine we are building on, before we
# build for the target
#
if [ -n "$UNSPOOF_PATH" ] 
then 
    ORIG_PATH=$PATH
    export PATH=$UNSPOOF_PATH
fi
if [ ! -f build-perl ]
then
    rm -rf nonx-build
    mkdir nonx-build
    cd nonx-build
    # Pre-determine the config values.  This idea is to match the system
    # we're building for rather than the host were building on. Obviously
    # because we're building a host version, this needs to be a linux
    # box, and similar to that of the target.  In a word this is a hack
    # to overcome Perls complete cross compiler unaware build system 
    # (I have read the INSTALL).  This should work most of the time.
    cat <<EOF > config.over
ccflags='-DNO_LOCALE -fno-strict-aliasing -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64'
ccsymbols='__GNUC_MINOR__=3 __GNUC_PATCHLEVEL__=2'
cf_email='support@freescale.com'
cppccsymbols='__GNUC__=3'
cppflags='-DNO_LOCALE -fno-strict-aliasing'
cppsymbols='__ELF__=1 _FILE_OFFSET_BITS=64 __GLIBC__=2 __GLIBC_MINOR__=3 __GNUC_MINOR__=3 __GNU_LIBRARY__=6 _LARGEFILE_SOURCE=1 _POSIX_C_SOURCE=199506 _POSIX_SOURCE=1 __STDC__=1 __USE_BSD=1 __USE_FILE_OFFSET64=1 __USE_LARGEFILE=1 __USE_MISC=1 __USE_POSIX=1 __USE_POSIX199309=1 __USE_POSIX199506=1 __USE_POSIX2=1 __USE_SVID=1 linux=1 __linux=1 __linux__=1 unix=1 __unix=1 __unix__=1'
d_charvspr='define'
d_fcntl_can_lock='undef'
d_pthread_yield='undef'
d_eaccess='undef'
dynamic_ext='B ByteLoader Cwd Data/Dumper Devel/DProf Devel/PPPort Devel/Peek Digest/MD5 Encode Fcntl File/Glob Filter/Util/Call I18N/Langinfo IO IPC/SysV List/Util MIME/Base64 Opcode POSIX PerlIO/encoding PerlIO/scalar PerlIO/via SDBM_File Socket Storable Sys/Hostname Sys/Syslog Time/HiRes Unicode/Normalize XS/APItest XS/Typemap attrs re threads threads/shared'
extensions='B ByteLoader Cwd Data/Dumper Devel/DProf Devel/PPPort Devel/Peek Digest/MD5 Encode Fcntl File/Glob Filter/Util/Call I18N/Langinfo IO IPC/SysV List/Util MIME/Base64 Opcode POSIX PerlIO/encoding PerlIO/scalar PerlIO/via SDBM_File Socket Storable Sys/Hostname Sys/Syslog Time/HiRes Unicode/Normalize XS/APItest XS/Typemap attrs re threads threads/shared Errno'
i_ndbm='undef'
libs='-lnsl -ldl -lm -lc -lcrypt -lutil'
libsfiles=' libnsl.so libdl.so libm.so libc.so libcrypt.so libutil.so'
libsfound=' /usr/lib/libnsl.so /usr/lib/libdl.so /usr/lib/libm.so /usr/lib/libc.so /usr/lib/libcrypt.so /usr/lib/libutil.so'
mydomain='.freescale.net'
myhostname='bladerunner'
myuname='linux bladerunner'
perladmin='support@freescale.com'
sig_count='65'
sig_name='ZERO HUP INT QUIT ILL TRAP ABRT BUS FPE KILL USR1 SEGV USR2 PIPE ALRM TERM STKFLT CHLD CONT STOP TSTP TTIN TTOU URG XCPU XFSZ VTALRM PROF WINCH IO PWR SYS NUM32 NUM33 NUM34 RTMIN NUM36 NUM37 NUM38 NUM39 NUM40 NUM41 NUM42 NUM43 NUM44 NUM45 NUM46 NUM47 NUM48 NUM49 NUM50 NUM51 NUM52 NUM53 NUM54 NUM55 NUM56 NUM57 NUM58 NUM59 NUM60 NUM61 NUM62 NUM63 RTMAX IOT CLD POLL UNUSED '
sig_name_init='"ZERO", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "BUS", "FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM", "STKFLT", "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU", "URG", "XCPU", "XFSZ", "VTALRM", "PROF", "WINCH", "IO", "PWR", "SYS", "NUM32", "NUM33", "NUM34", "RTMIN", "NUM36", "NUM37", "NUM38", "NUM39", "NUM40", "NUM41", "NUM42", "NUM43", "NUM44", "NUM45", "NUM46", "NUM47", "NUM48", "NUM49", "NUM50", "NUM51", "NUM52", "NUM53", "NUM54", "NUM55", "NUM56", "NUM57", "NUM58", "NUM59", "NUM60", "NUM61", "NUM62", "NUM63", "RTMAX", "IOT", "CLD", "POLL", "UNUSED", 0'
sig_num='0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 6 17 29 31 '
sig_num_init='0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 6, 17, 29, 31, 0'
sig_size='69'
timeincl='sys/time.h'
yacc='yacc'
perllibs='-lnsl -ldl -lm -lc -lcrypt -lutil'
EOF
    # configure: config.over will override the guesses
    sh ../Configure -O -Dmksymlinks \
    -Dinstallprefix=$RPM_BUILD_ROOT/%{pfx}/%{_prefix} -Dprefix=%{_prefix} \
    -des -Accflags=-DNO_LOCALE -Dcc=gcc 
    make miniperl perl
    cd -
    for i in miniperl perl
    do
       cp nonx-build/$i build-$i
    done
fi
#
# build a cross version
#
export PATH=$ORIG_PATH
for i in config.sh Policy.sh
do
   cp nonx-build/$i $i
done
./build-perl -pi.orig -e '
                s,\./miniperl,./build-miniperl,g;
                s,\./perl,./build-perl,g;
                                                  ' Makefile.SH
./build-perl -pi.orig -e '
                s,/miniperl,/build-miniperl,g;
                                                  ' x2p/Makefile.SH
./build-perl -pi.orig -e '
                s,/miniperl,/build-miniperl,g;
                                                  ' ext/util/make_ext
./build-perl -pi.orig -e '
                s,/miniperl,/build-miniperl,g;
                                                  ' utils/Makefile
./build-perl -pi.orig -e '
                s,die "You must run as root,warn "You must run as root,;
                                                  ' installperl
./build-perl -pi.orig -e '
                s,$ENV{BUILDARCH},$ENV{LINTARCH},g;
                s,^xlibpth=.*,xlibpth="/usr/lib/$ENV{LINTARCH} /lib/$ENV{LINTARCH}",;
                ### maybe we need to do this ? s,^byteorder=.*,,;
                s,^full_ar=.*,full_ar="ar",;
                                                  ' config.sh

if [ ! -f ext/File/Glob/Glob.pm.orig ]
then
    mv ext/File/Glob/Glob.pm ext/File/Glob/Glob.pm.orig
    cp -f %{SOURCE1} ext/File/Glob/Glob.pm.simple
    cp ext/File/Glob/Glob.pm.simple ext/File/Glob/Glob.pm
fi

# there is something wrong with the makedepends, this is harmless and
# fixes the problem.  It only occurs on re-started builds
touch wince.h nwutil.h

./Configure -S
make depend
make


%Install
rm -rf $RPM_BUILD_ROOT
make install
cp -af ext/File/Glob/Glob.pm.orig $RPM_BUILD_ROOT/%{pfx}/%{_prefix}/lib/perl5/%{version}/${LINTARCH}-linux/File/Glob.pm


%Clean
rm -rf $RPM_BUILD_ROOT


%Files
%defattr(-,root,root)
%{pfx}/*


