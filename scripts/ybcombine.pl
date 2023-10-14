#!/usr/bin/perl
#
# combine Marble sprites [PRS]

use strict;

our $CONVERT = 'C:\usr\ImageMagick\composite.exe';

unless (@ARGV == 1) {
    print "usage: ybcombine.pl _SPRSET\n";
    exit 0;
}

$/ = "\0";
while (<>) {
    chomp;
    /^:(.*)/ or next;
    last if $1 eq 'END';
    my $target_name = "../${1}.png";
    my $base = <>;
    chomp $base;
    $_ = <>;
    chomp;
    my ($overlay, $x, $y) = split /,/;
    if (-e $target_name) {
        warn "${target_name}: target already exists\n";
        next;
    }
    my $base_name = $base.'.png';
    unless (-e $base_name) {
        $base_name = '~'.$base_name;
        next unless -e $base_name;
    }
    my $geometry = sprintf ('%+d%+d', $x, $y);
    my @cmd = ("${overlay}.png", '-geometry', $geometry, ${base_name}, $target_name);
#    print join(' ', 'composite', @cmd), "\n";
    print "$base + $overlay -> $target_name\n";
    system $CONVERT, @cmd;
    rename ${base_name}, "~${base_name}" unless $base_name =~ /^~/;
}
