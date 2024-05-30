######################################################################
#
# Copyright © Freescale Semiconductor, Inc. 2004-2005. All rights reserved.
#
# Stuart Hughes, stuarth@freescale.com,  22nd Feb 2005
#   
# This file is part of LTIB.
#
# LTIB is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# LTIB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LTIB; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# Description:
#
# Utility functions loaded by LTIB
#
# CVS version: $Revision$
#
######################################################################
package Ltibutils;
require 5.003;
require Exporter;

use File::Find;
use File::Basename;

# Test for optional modules
BEGIN {
    $have_md5 = 0;
    if( eval "require Digest::MD5" ) {
        Digest::MD5->import();
        $have_md5 = 1;
    } else {
        warn("Don't have Digest::MD5, will fallback to md5sum\n");
    }
    $have_request_common = 0;
    if( eval "require HTTP::Request::Common" ) {
        HTTP::Request::Common->import(HEAD);
        $have_request_common = 1;
    } else {
        warn("Don't have HTTP::Request::Common\n");
    }
    $have_useragent = 0;
    if(eval "require LWP::UserAgent") {
        $have_useragent = 1;
    } else {
        warn("Don't have LWP::UserAgent\n");
    }
    warn( "Cannot test proxies, or remote file availability without both\n"
         ."HTTP::Request::Common and LWP::UserAgent\n")
                               unless $have_request_common && $have_useragent;
}

use vars qw(@ISA @EXPORT);
@ISA = qw(Exporter);
@EXPORT = qw(hash_from_file
             parse_dotconfig hash_to_config_in
             gm_yyyymmdd parse_config
             parse_spec get_file touch g2larch
             get_ver cmp_ver mk_uboot_kernel mk_fs_image
             cmd_w_to system_nb get_cvs_tags caller_stack);


# This is a general version of parse_dotconfig
sub hash_from_file
{
    my ($file, $hr) = @_;
    open(FH, $file) or die("open(FH, $file) : $!\n");
    while(<FH>) {
        chomp;
        m,^([\w]+)\s*=\s*("?[^"]+"?), or next;
        $hr->{$1} = $2;
    }
    close FH;
    while($main::cf->{verbose} && (my($k,$v) = each %$hr)) { warn "'$k'='$v'\n" };
    return 1;
}

sub parse_dotconfig
{
    my($f) = @_;
    my $hr = {};
    warn("parsing $f\n") if $main::cf->{verbose};
    open($fh, $f) or warn("parse_dotconfig: line ",  (caller())[2],
                           ", open $f: $!\n"), return;
    while(<$fh>) {
        chomp;
        my ($k, $v) = m,(CONFIG_[^\s=]+)[\s=]+(.*),;
        next unless $k;
        $k =~ s,\bCONFIG_,,;
        $v =~ s,is not set,,;
        # strip off the enclosing quotes
        $v =~ s,^",,;
        $v =~ s,"$,,;
        $hr->{$k} = $v;
    }
    close $fh;
    while($main::cf->{verbose} && (my($k,$v) = each %$hr)) { warn "$k=$v\n" };
    return $hr;
}

# beware, this is probably broken as I strip of in-comming quotes 
# when reading in the .config files
sub hash_to_config_in
{
    my($hr, $file) = @_;
    my $date = gm_yyyymmdd();
    open(FH, ">$file") or die("open(FH, >$file) : $!\n");
    print FH <<TXT;
# Automatically generated make ltib_config: don't edit
# Date: $date
#
TXT
    while(my($k,$v) = each %$hr) {
        $type = $v =~ m,^", ? "string" : "bool";
        print FH <<TXT;
config $k
    $type
    default $v
TXT
    }
    close FH;
    return 1;
}

sub gm_yyyymmdd
{
    my($day, $month, $year) = (gmtime)[3,4,5];
    return sprintf("%04d%02d%02d", $year+1900, $month+1, $day);
}

sub parse_config
{
    my ($f, $hr, $parse_opts) = @_;
    die "not enough args" unless $hr;
    my ($tok, $tok_name, $ld, $fh) = ("", "", 0);
    local $_;
    open($fh, $f) or warn("open $f: $!\n"), return;
    while(<$fh>) {
        if( eof || /^%([\w-]+)/ ) {
            if( $ld ) {
                $tok .= $_ if eof;
                $tok =~ s/\s*$//;
                $hr->{$tok_name} = $tok;
                $ld = 0;
            }
            $tok_name = $1;
            $ld       = 1;
            $tok      = "";
        } else {
            $tok .= $_ if $ld;
        }
    }
    if(defined $parse_opts) {
        for $tok ( keys %$hr ) {
            next unless defined $hr->{$tok};
            if($parse_opts->{strip_comment}) {
                $hr->{$tok} =~ s,^\s*#.*\n?,,gm;
            }
            if($parse_opts->{strip_blank}) {
                $hr->{$tok} =~ s,^\s*\n,,gm;
            }
            if($parse_opts->{strip_trailing}) {
                $hr->{$tok} =~ s,\s+$,,gm;
            }
        }
    }
    close $fh;
    while($main::cf->{verbose} && (my($k,$v) = each %$hr)) { warn "$k=$v\n" };
    return $hr;
}

sub parse_spec
{
    my ($specname, $tokens, $no_reduce, $mode) = @_;
    my $defines  = {};
    $mode ||= '';

    # Read in the whole file in one go. Okay as spec files are small
    local $/ = undef;
    open(SPEC, $specname) or warn("can't open $specname : $!\n"), return;
    local $_ = <SPEC>;
    close SPEC;

    # match and extract defines/tokens
    m,^name\s*:\s*(.+),mi       and do { $tokens->{name} = $1 };
    return if $mode eq 'name';
    m,^%define\s+base\s+(.+),m and do { $tokens->{base}  = $1 };
    m,^%define\s+pfx\s+([\S]+),m and do { $tokens->{pfx}  = $1 };
    m,^version\s*:\s*(.+),mi    and do { $tokens->{version} = $1 };
    m,^release\s*:\s*(.+),mi    and do { $tokens->{release} = $1 };
    m,^buildroot\s*:\s*(.+),mi  and do { $tokens->{buildroot} = $1 };
    while(m,^source\d*\s*:\s*(.+)$,mig) { $tokens->{sources} .= "$1\n" };
    while(m,^patch\d*\s*:\s*(.+)$,mig ) { $tokens->{patches} .= "$1\n" };
    m,^license\s*:\s*(.+)$,mi   and do { $tokens->{license} = $1 };
    m,^summary\s*:\s*(.+)$,mi   and do { $tokens->{summary} = $1 };
    m,^%setup\s+.*-n\s+(.+)$,mi   and do { $tokens->{pkg_dir_name} = $1 };
    m,^%prep(\s*.+?)(?:(^\s*$)|\Z),msi and do { $tokens->{prep} = $1 };
    m,^%build\s*(.+?)^\s*$,msi  and do { $tokens->{build} = $1 };
    m,^%install\s(.+?)^\s*$,msi and do { $tokens->{install} = $1 };
    m,^%files(\s*.+?)(?:(^\s*$)|\Z),msi and do { $tokens->{files} = $1 };

    # derive the directory name the package will build into
    $tokens->{pkg_dir_name} ||= "$tokens->{name}-$tokens->{version}";

    # extract a list of %defines and create a hash from the key/values
    foreach my $k (qw/name version release/) {
        $defines->{$k} = $tokens->{$k};
    }
    $defines->{$1} = $2 while m,^%define\s+([^\s]+)\s+([^\s]+)$,gm;

    # reduce defines
    unless($no_reduce) {
        reduce_hash($defines, $defines);
        reduce_hash($tokens, $defines);
    }
    while($main::cf->{verbose} && (my($k,$v) = each %$tokens)) { warn "$k=$v\n" };
}

sub reduce_hash
{
    my ($to_reduce, $vars) = @_;
    my ($count, $val, $has_vars, $key) = (0);
    local ($_, $1);

    while($count < 1000) {
        warn("can't complete variable reduction\n"), return if $count++ > 200;

        $has_vars = 0;
        foreach $key (keys %$to_reduce) {
            $_ = $to_reduce->{$key};
            if(! defined($_) ) {
                warn "key $key had no value\n";
            }
            m,%{([^}]+)}, and do {
                $has_vars = 1;
                undef $val;
                $val = $vars->{$1};
                chop($val = `rpm --eval \%$1`) unless defined($val);
                if(defined($val)) {
                    s,%{$1},$val,g;
                    $to_reduce->{$key} = $_;
                }
            }
        }
        last if $has_vars == 0;
    }
}


sub get_file
{
    my ($url, $cf, $force_md5) = @_;
    warn("no url passed"), return unless $url;
    warn("lpp: $cf->{lpp} is not a directory"), return unless -d $cf->{lpp};
    my ($path,  $refmd5);
    my ($file)  = $url =~ m-/?([^/]+)$-;
    my @md5s    = ("$file.md5");

    # This is needed to let LWP::UserAgent cycle through the authentication
    # types in the test proxy section (called from here lower down)
    local $SIG{__DIE__} = 'DEFAULT';

    # only test the file if in test mode
    return test_remote_file($url, $cf) if $cf->{dltest};

    # Don't check md5sums if file is local unless forced.  The rationale is
    # that it would be checked when downloaded, and if not downloaded, we
    # may not want to do network accesses.
    my @ldirs = grep { ! m,^\s*$, && ! m,\s*#, } split(/\s+/, $cf->{ldirs});
    foreach my $dir ($cf->{lpp}, @ldirs, "$cf->{top}/pkgs") {
        $path = "$dir/$file";
        if(-f $path) {
            return $path unless $force_md5;

            # try to get the reference md5sum for this file
            $refmd5 = get_ref_md5($file, $cf, @md5s);
            return md5sum_ok($path, $refmd5) ? $path : undef;
        }
    }
    
    # try to get the reference md5sum for this file
    $refmd5 = get_ref_md5($file, $cf, @md5s) unless $refmd5;

    # try to get the file from various remote locations
    $path = get_remote_file($url, $cf) or return;

    # if we got the file, verify the md5sum
    return md5sum_ok($path, $refmd5) ? $path : undef;
}

my $proxy_tested = 0;
sub test_proxy
{
    my ($cf) = @_;

    return if $proxy_tested;
    $proxy_tested = 1;
    return if $cf->{bypass_network_tests};
    return unless $have_request_common && $have_useragent;
    return unless $cf->{proxy};
    if($cf->{proxy} eq 'off') {
        $cf->{proxy} = 0;
        return;
    }
    if(!$cf->{http_proxy}) {
        print ("No http_proxy has been set, proxy forced off\n");
        $cf->{proxy} = 0;
        return;
    }
    return unless $cf->{gpp_url};

    print "Testing proxy connection\n";
    my $ua = LWP::UserAgent->new;
    my $req = HEAD($cf->{gpp_url});
    $ua->proxy('http', $cf->{http_proxy});
    my $res = $ua->request($req);
    return if $res->is_success;
    print("Testing GPP with proxies failed, response was: ",
                                                  $res->status_line, "\n");
    print("Can't get GPP index, proxy forced off\n");
    $cf->{proxy} = 0;
    return;
}

my $ppp_gpp_tested = 0;
sub test_ppp_gpp
{
    my ($cf) = @_;

    return 1 if $ppp_gpp_tested;
    $ppp_gpp_tested = 1;
    return unless $cf->{ppp_url} || $cf->{gpp_url};
    return 1 if $cf->{bypass_network_tests};
    return 1 unless $have_request_common && $have_useragent;

    # test the proxy connection
    my $ret = test_proxy($cf);

    # test the PPP and GPP connections, the PPP is never proxied
    my $ua = LWP::UserAgent->new;
    foreach my $url ( qw/ppp_url gpp_url/ ) {
        next unless $cf->{$url};
        print("Testing $url connectivity: ");
        my $req = HEAD($cf->{$url});
        if($url eq 'gpp_url' && $cf->{proxy}) {
            $ua->proxy('http', $cf->{http_proxy}) if $cf->{proxy};
        }
        my $res = $ua->request($req);
        print("OKAY\n"), next if $res->is_success;
        print("FAIL, disabling\n");
        $cf->{$url} = '';
    }
    return ($cf->{ppp_url} || $cf->{gpp_url}) ? 1 : undef;
}

sub test_remote_file
{
    die "Some modules to test remote file access are missing"
                               unless $have_request_common && $have_useragent;
    my ($url, $cf) = @_;
    warn("no url passed"), return unless $url;
    my ($file) = $url =~ m-/?([^/]+)$-;
    my $ua  = LWP::UserAgent->new;
    my ($req, $res);

    # test the ppp/gpp connection
    test_ppp_gpp($cf)
                 or warn("No network download connection available\n"), return;
    
    # try to get from the PPP
    if($cf->{ppp_url}) {
        warn "testing on PPP" if $main::cf->{verbose};
        $req  = HEAD("$cf->{ppp_url}/$file");
        $res = $ua->request($req);
        print("OK PPP: $file\n"), return 1 if $res->is_success;      
    }
    print("FAILED: $file\n"), return unless $cf->{gpp_url};

    # try to get from the GPP
    $req = HEAD("$cf->{gpp_url}/$file");
    if($cf->{proxy}) {
        warn "testing on GPP with proxies" if $main::cf->{verbose};
        $ua->proxy('http', $cf->{http_proxy}) if $cf->{proxy};
    } else {
        warn "testing on GPP without proxies" if $main::cf->{verbose};
    }
    $res = $ua->request($req);
    print("OK GPP: $file\n"), return 1 if $res->is_success;      
    print("FAILED: $file ", $res->status_line, "\n");
    return;
}

sub get_remote_file
{
    my ($url, $cf, $dest) = @_;
    return test_remote_file($url, $cf) if $cf->{dry};
    warn("no url passed"), return unless $url;
    $dest ||= $cf->{lpp};
    warn("dest (lpp): $dest is not a directory"), return unless -d $dest;
    my ($file)  = $url =~ m-/?([^/]+)$-;
    my $path = "$dest/$file";

    # test the ppp/gpp connection
    test_ppp_gpp($cf)
                 or warn("No network download connection available\n"), return;

    # try to get from the PPP
    if($cf->{ppp_url}) {
        print "Getting $file from the Private Package Pool\n";
        system_nb(<<TXT);
cd $dest
wget $cf->{wget_opts} --proxy=off $cf->{ppp_url}/$file 2>&1
cd -
TXT
        return $path if -f $path;
    }

    # try to get from the gpp without proxies regardless (we may not
    # have been able to test proxy access due to a missing module
    if($cf->{gpp_url}) {
        print "Getting $file from the Global Package Pool\n";
        my($pxys, $pxmode) = ("", "off");
        if($cf->{proxy}) {
            $pxys = "http_proxy=$cf->{http_proxy} ftp_proxy=$cf->{ftp_proxy}";
            $pxmode = "on";
        }
        system_nb(<<TXT);
cd $dest
$pxys wget $cf->{wget_opts} --proxy=$pxmode $cf->{gpp_url}/$file 2>&1
cd - >/dev/null
TXT
        return $path if -f $path;
    }

    return;
}

$md5_dl_fail = 0;
$updated_md5 = 0;
sub get_ref_md5
{
    my($file, $cf, @md5s) = @_;
    my ($dest, $refmd5, $fn, $md5, $md5file, $path) 
                                      = ($cf->{lpp}, "", "", "", "", "");

    # look for and check local files first
    foreach $fn (@md5s) {
        $md5file = $fn, last if -f "$dest/$fn";
    }
    # if no local files, try to get a remote file from gpp
    if(! $md5file && ! $md5_dl_fail) {
        foreach $fn (@md5s) {
            if( get_remote_file($fn, $cf) ) {
                $md5file = $fn;
                last;
            }
        }
        $updated_md5 = 1;
        $md5_dl_fail = 1 unless -f "$dest/$fn";
    }
    # give up if there is no md5 file available
    return "" unless $md5file;

    # if the md5 file exists, look for the file's md5sum in there
    open(MD5, "$dest/$md5file") or warn("open $dest/$md5file: $!\n"), return;
    while(<MD5>) {
        ($md5, $fn) = split(/\s+/);
        if($fn eq $file) {
            $refmd5 = $md5;
            last;
        }
    }
    close MD5;
    if( !$refmd5 && ! $updated_md5) {
        unlink("$dest/$md5file");
        # recursively call ourselves to force an update of the md5file
        $refmd5 = &get_ref_md5;
        warn "WARNING: could not get md5 reference for $file\n\n" if ! $refmd5;
    }
    return $refmd5;
}

sub md5sum_ok
{
    my ($path, $refmd5) = @_;
    return 1 unless $refmd5;
    warn "verifying md5sum for $path\n" if $main::cf->{verbose}; 
    return 1 if md5sum($path) eq $refmd5;

    warn "ERROR: md5sum mismatch, removing $path, please re-try\n";
    unlink $path;
    return;
}
    
sub md5sum
{
    my ($path) = @_;
    if(! $have_md5 ) {
        my ($md5) = split(/\s+/, `md5sum $path`);
        return $md5;
    }
    open(F, $path) or warn("can't open $path: $!\n"), return;
    binmode(F);
    return Digest::MD5->new->addfile(*F)->hexdigest;
}


sub touch
{
    $now = time();
    foreach my $file (@_) { 
        utime($now, $now, $file)
          or open($file, ">>$file"), close($file)
          or warn("Can't touch $file\n"), next;
    }
    return 1;
}

sub g2larch
{
    my ($gnuarch) = @_;
    my $arches = {
        powerpc => "ppc",
    };
    if( exists($arches->{$gnuarch}) ) {
        return $arches->{$gnuarch};
    } 
    return $gnuarch;
}

sub cmp_ver
{
    my @ver = split(/\./, $_[0]);
    my @min = split(/\./, $_[1]);

    for my $ref (@ver) {
        return -1 if $ref < $min[0];
        return +1 if $ref > $min[0];
        shift(@min);
        last unless $min[0];
    }
    return 0;
}


$app_checks = {
    binutils         => 'ar --version 2>/dev/null',
    'gcc-c++'        => 'g++ --version 2>/dev/null',
    glibc            => 'ldd --version 2>/dev/null',
    'glibc-devel'    => sub { -f '/usr/lib/libm.so' },
    'glibc-headers'  => sub { -f '/usr/include/stdio.h' },
    'libstdc++' => sub { my @f = glob('/usr/lib/libstdc++*'); @f > 1 ? 1 : 0 },
    'libstdc++-devel'=> sub { return system_nb("ls -ld /usr/include/[gc]++* &>/dev/null") == 0 },
    lkc              => 'mconf -h 2>/dev/null',
    ncurses          => 'tic -V 2>/dev/null',
    'ncurses-devel'  => sub { -f '/usr/include/ncurses.h' },
    'rpm-build'      => sub { -f '/usr/lib/rpm/rpmb' },
    sudo             => 'sudo -V 2>/dev/null',
    texinfo          => sub {  
                       warn  "WARNING: you may also need to install:"
                            ." tetex-fonts, dialog and textex\n" 
                         unless  -f '/usr/share/texmf/tex/texinfo/texinfo.tex';
                        `makeinfo --version 2>/dev/null`;
                    },
    zlib         => sub { my @f = (glob('/usr/lib/libz.so*'), 
                                   glob('/lib/libz.so*')    ); @f > 1 ? 1 : 0 },
    'zlib-devel' => sub { -f '/usr/include/zlib.h' },
};

sub get_ver
{
    my ($pkg) = @_;
    local $_;

    if( ! defined($app_checks->{$pkg}) ) {
        $_ = `$pkg --version 2>/dev/null`;
    } elsif(ref($app_checks->{$pkg}) eq 'CODE') {
        $_ = $app_checks->{$pkg}();
    } else {
        $_ = `$app_checks->{$pkg}`;
    }
    if(! $_) {
        return (-1, 'not installed');
    }
    ($ver) = m,(\d+\.\d+(?:\.\d+)?),;
    $ver ||= 0;
    return $ver;
}

sub mk_uboot_kernel
{
    my ($vmlinuz, $pcf) = @_;
    return 1 unless $pcf->{DEPLOYMENT_U_BOOT_KERNEL};
    my $outpath = $pcf->{DEPLOYMENT_U_BOOT_KERNEL_PATH} || "vmlinux.gz.uboot";

    print "making vmlinux.gz.uboot\n";
    my $x = $main::cf->{verbose} ? '-x' : '';
    system_nb(<<TXT) == 0 or return;
set -e
if [ -n "$x" ]; then set $x ; fi

rm -f vmlinux.gz.uboot

mkimage -n 'Linux for $pcf->{PLATFORM}' \\
        -A ppc -O linux -T kernel -C gzip \\
        -d $vmlinuz $outpath
TXT
    return 1;
}

sub mk_fs_image
{
    my($rootfs, $stage, $dev_tab, $pcf) = @_;
    return 1 if $pcf->{DEPLOYMENT_NFS};

    warn("$rootfs directory missing"), return unless -d $rootfs;
    warn("device table: $dev_tab missing"), return unless -e $dev_tab;

    my $tdir = $pcf->{DEPLOYMENT_ROOTFS_DIR} || ".";
    warn("target dir '$tdir' does not exist\n"), return unless -d $tdir;

    # this turns of Use of uninitialized value in concatenation warnings
    local $^W = 0;

    print "making filesystem image file\nstaging directory is $stage\n";

    my $x = $main::cf->{verbose} ? '-x' : '';
    my $v = $main::cf->{verbose} ? '-v' : '';
    my $cmd;

    system_nb(<<TXT);
set -e
if [ -n "$x" ]; then set $x ; fi
rm -rf $stage
if [ "$pcf->{DEPLOYMENT_RAMDISK}" = "y" ]
then
    rm -f $tdir/rootfs.ext2 $tdir/rootfs.ext2.gz $tdir/rootfs.ext2.gz.uboot
fi
if [ "$pcf->{DEPLOYMENT_JFFS2}" = "y" ]
then
    rm -f $tdir/rootfs.jffs2
fi
if [ "$pcf->{DEPLOYMENT_CRAMFS}" = "y" ]
then
    rm -f $tdir/cramfs.*
fi
if [ "$pcf->{DEPLOYMENT_ROMFS}" = "y" ]
then
    rm -f $tdir/image.bin $tdir/tmp/romfs.img $tdir/image.bin.gz
fi
    mkdir -p $stage
TXT

    # copy the rootfs directory tree to the staging area
    opendir(L1, $rootfs) or die("opendir $rootfs: $!\n");
    my $l1;
    while( defined($l1 = readdir(L1)) ) {
        next if $l1 eq '.' || $l1 eq '..';
        if( $l1 eq 'dev') {
           print "mkdir $stage/$l1\n" if $v;
           mkdir("$stage/$l1") or die "mkdir $stage/$l1 : $!";
           opendir(L2, "$rootfs/$l1") or die("opendir $rootfs/$l1: $!\n");
           my $l2;
           while( defined($l2 = readdir(L2)) ) {
               next if $l2 eq '.' || $l2 eq '..';
               if(-d "$rootfs/$l1/$l2") {
                   print "mkdir $stage/$l1/$l2\n" if $v;
                   mkdir "$stage/$l1/$l2" or die "mkdir $stage/$l1/$l2 : $!";
               }
           }
           closedir(L2);
           next;
        }
        $cmd = "cp -a $rootfs/$l1 $stage/";
        print "$cmd\n" if $v;
        system($cmd)
    }
    closedir(L1);

    # cleanup the staging area
    system_nb(<<TXT) == 0 or return;
set -e 
if [ -n "$x" ]; then set $x ; fi
if [ "$pcf->{DEPLOYMENT_RM_BOOT}" = "y" ]
then
    echo "removing the boot directory and files"
    rm -rf $stage/boot
fi
if [ "$pcf->{DEPLOYMENT_RM_DOCS}" = "y" ]
then
    echo "removing man files and directories"
    rm -rf $stage/usr/share/man
    ####rm -rf $stage/usr/man
    echo "removing info files"
    rm -rf $stage/usr/info
fi
if [ "$pcf->{DEPLOYMENT_RM_USR_SRC}" = "y" ]
then
    echo "removing /usr/src directory"
    rm -rf $stage/usr/src
fi
if [ "$pcf->{DEPLOYMENT_RM_USR_INCLUDE}" = "y" ]
then
    echo "removing /usr/include directory"
    rm -rf $stage/usr/include
fi
if [ "$pcf->{DEPLOYMENT_RM_USER_DIRS}" != "" ]
then
    for i in $pcf->{DEPLOYMENT_RM_USER_DIRS}
    do
        if [ -d $stage/\$i ]
        then
            echo "removing \$i directory"
            rm -rf $stage/\$i
        fi
    done
fi
if [ "$pcf->{DEPLOYMENT_RM_USER_FILES}" != "" ]
then
    for i in $pcf->{DEPLOYMENT_RM_USER_FILES}
    do
        if [ -e $stage/\$i ]
        then
            echo "removing file \$i"
            rm -f $stage/\$i
        fi
    done
fi
if [ "$pcf->{DEPLOYMENT_RM_STATIC_LIBS}" = "y" ]
then
    echo "removing static libraries"
    find $stage -name \\*.a -exec rm -f {} \\;
fi
if [ "$pcf->{DEPLOYMENT_STRIP}" = "y" ]
then
    echo "stripping binaries and libraries"
    stripall -s $v $stage
fi
TXT

    # calculate the size of the rootfs (based on calcs in buildroot)
    my $fs_count = 0;
    find( sub { $fs_count++ }, $stage);
    my ($fs_size) =  split(/\s+/, `LANG=C du -slk $stage`);
    $fs_size += $fs_size >= 20000 ? 16384 : 2400;
    $fs_size += $pcf->{DEPLOYMENT_PADDING_KB} if $pcf->{DEPLOYMENT_PADDING_KB};
    print <<TXT;

Filesystem stats, including padding:

    Total size            = ${fs_size}k
    Total number of files = $fs_count

TXT
    my $fs_size_p10 = int($fs_size * 1.1);

    if($fs_size > 4096) {
        print <<TXT if $pcf->{DEPLOYMENT_RAMDISK};
Your ramdisk exceeds the old default size of 4096k, you may need to
set the command line argument for ramdisk_size in your bootloader
allowing 10% free this gives ${fs_size_p10}k .  For instance, for u-boot:


setenv bootargs root=/dev/ram rw ramdisk_size=$fs_size_p10

TXT
    }
    # genext2fs seems to use 1k block size
    my $blocks = $fs_size;
    my $inodes = $fs_count + 400;

    # try to sanely guess the endian of the target
    my $endian = '-l';
    $pcf->{ENDIAN} ||= "b";
    $endian = "-b" if $pcf->{ENDIAN} =~ /b/i;
    warn "ENDIAN=$pcf->{ENDIAN}, endian=$endian\n" if $main::cf->{verbose};

    system_nb(<<TXT) == 0 or return;
if [ -n "$x" ]; then set $x ; fi
set -e 
if [ "$pcf->{DEPLOYMENT_RAMDISK}" = "y" ]
then
    echo "creating an ext2 compressed filesystem image: rootfs.ext2.gz"
    genext2fs -U -b $blocks -i $inodes -D $dev_tab -d $stage $tdir/rootfs.ext2
    gzip $tdir/rootfs.ext2
fi
if [ "$pcf->{DEPLOYMENT_RAMDISK_U_BOOT}" = "y" ]
then
    echo "creating a uboot ramdisk image: rootfs.ext2.gz.uboot"
    mkimage -n 'uboot ext2 ramdisk rootfs' \\
        -A ppc -O linux -T ramdisk -C gzip \\
        -d $tdir/rootfs.ext2.gz $tdir/rootfs.ext2.gz.uboot
fi
if [ "$pcf->{DEPLOYMENT_JFFS2}" = "y" ]
then
    mkfs.jffs2 -n -p -D $dev_tab -U $endian -e $pcf->{DEPLOYMENT_ERASE_BLOCK_SIZE} -d $stage -o $tdir/rootfs.jffs2
fi
if [ "$pcf->{DEPLOYMENT_CRAMFS}" = "y" ]
then
    if [ "$endian" = "-b" ]
    then
        echo "creating big-endian compressed ramfs: cramfs.big"
        mkfs.cramfs -D $dev_tab -r $stage $tdir/cramfs.big
    fi
    if [ "$endian" = "-l" ]
    then
        echo "creating little-endian compressed ramfs: cramfs.little"
        mkfs.cramfs -D $dev_tab $stage $tdir/cramfs.little
    fi
fi
if [ "$pcf->{DEPLOYMENT_ROMFS}" = "y" ]
then
    echo "creating romfs filesystem"
    for device in \$(tr -cs '[:graph:]' '[\n*]' < bin/device_genromfs.txt); \\
        do touch $stage/dev/\@\$device; done
    ln -s /var/tmp/log $stage/dev/log
    ln -s /proc/mounts $stage/dev/mtab
    genromfs -V "ROMdisk" -d $stage -f $tdir/tmp/romfs.img
    cat $rootfs/boot/bootable_kernel $tdir/tmp/romfs.img > $tdir/image.bin
    if [ "$pcf->{DEPLOYMENT_ZIP}" = "y" ]
    then
        echo "compressing romfs/kernel filesystem"
        gzip -c -8 $tdir/image.bin > $tdir/image.bin.gz
    fi
    if [ "$pcf->{DEPLOYMENT_SREC}" = "y" ]
    then
        echo "creating srec files"
        objcopy -O srec -I binary $tdir/image.bin $tdir/image.bin.srec
        if [ "$pcf->{DEPLOYMENT_ZIP}" = "y" ]
        then
            objcopy -O srec -I binary $tdir/image.bin.gz $tdir/image.bin.gz.srec
        fi
    fi
fi
if [ "$pcf->{DEPLOYMENT_ROOTFS_KEEPSTAGE}" = "y" ]
then
    echo "Saving temporary staging directory: $stage"
else
    rm -rf $stage
fi
TXT
    return 1;
}

sub cmd_w_to
{
    my($timeout, @cmd) = @_;
    my $cmd_op;

    local $SIG{ALRM} = sub { die "timeout" };
    eval {
        alarm($timeout);
        $cmd_op = `@cmd`;
        alarm(0);
    };
    return $cmd_op unless $@;
    warn("caught timeout: @cmd\n"), return if $@ =~ /timeout/;
    warn("$@: @cmd\n"), return;
}

# Normally system will block SIGINT and SIGQUIT
# We don't want to do this, or we can't properly CNTRL-C ltib
sub system_nb
{
    my (@cmd) = @_;
    if(my $pid = fork) {
        waitpid($pid, 0);
        return $?;
    } else {
        die "cannot fork: $!\n" unless defined $pid;
        exec(@cmd) or die "exec: @cmd: $!";
    }
}

#
# Get a list of cvs tags, the return list
# is expected to be latest tag first.
#
sub get_cvs_tags
{
    my ($tags, @tags);
    open(CVSLOG, "cvs log 2>/dev/null |") or die;
    while(<CVSLOG>) {
        if( m,^\s+([^\s]+):\s+\d,) {
            $tags->{$1} += 1;
            if($tags->{$1} == 1) {
                push @tags, $1;
            }
        }
    }
    close CVSLOG;
    return @tags;
}

sub caller_stack
{
    my $i = 0;
    my ($pack, $savline, $line, $subname);
    my $ci = "";
    while( ($pack, undef, $savline, $subname) = caller($i)) {
        $ci .= " " x (($i -1)*2) . "$pack:$line:$subname\n" if $line;
        $line = $savline;
        $i++;
    }
    return $ci;
}
1;

