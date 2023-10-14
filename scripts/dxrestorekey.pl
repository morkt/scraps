#!/usr/bin/perl
#
# restore DxLib key

use strict;

my @key = (0x9d, 0xc6, 0xe5, 0x09, 0x9b, 0xcf, 0x90, 0x12, 0x5b, 0x16, 0x80, 0xae);

sub RotByteR {
    my ($val, $shift) = @_;
    $shift &= 7;
    return ($val >> $shift | $val << (8 - $shift)) & 0xFF;
}

sub RotByteL {
    my ($val, $shift) = @_;
    $shift &= 7;
    return ($val << $shift | $val >> (8 - $shift)) & 0xFF;
}

$key[0] ^= 0xFF;
$key[1]  = RotByteL ($key[1], 4);
$key[2] ^= 0x8A;
$key[3]  = RotByteL ($key[3] ^ 0xFF, 4);
$key[4] ^= 0xFF;
$key[5] ^= 0xAC;
$key[6] ^= 0xFF;
$key[7]  = RotByteL ($key[7] ^ 0xFF, 3);
$key[8]  = RotByteR ($key[8], 3);
$key[9] ^= 0x7F;
$key[10] = RotByteL ($key[10] ^ 0xD6, 4);
$key[11] ^= 0xCC;

my $key_str = join ('', map { chr $_ } @key);
print $key_str, "\n";
