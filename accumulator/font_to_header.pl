#!/usr/bin/perl -w
use strict;
use Data::Dumper;

my $in=$ARGV[0];
my $font_hight=$ARGV[1];
my $count=0;
my $asc=0;
print STDERR $in;
my $F;
open ($F,'<',$in);
print STDOUT "#include <avr/pgmspace.h>\n";
print STDOUT "prog_char ".(split /\./,$in)[0]."[][".$font_hight."] PROGMEM =\n{\n";


while(defined(my $char=getc($F))) {
	if(($count % $font_hight)==0) {
		if($count>0) {
			print STDOUT ",\n";
			$asc++;
		}
		print STDOUT "\t{ /*";
		print STDOUT "\t".$asc;
		if($asc > 31 && $asc < 128) {
			print STDOUT "\t'".chr($asc)."'";
		}
		print STDOUT " */\n"
	}
	$count++;
	my $byte=ord $char;
	print STDOUT "\t\t0b";
	for(my $i=0;$i<8;$i++,$byte <<=1) {
		print STDOUT $byte & 0x80 ? "1" : "0";
	}
	if(($count % $font_hight)==0) {
		print STDOUT "\n\t}";
	} else {
		print STDOUT ",\n";
	}
}
print STDOUT "\n};\n";
