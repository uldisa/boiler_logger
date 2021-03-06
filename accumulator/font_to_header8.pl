#!/usr/bin/perl -w
use strict;
use Data::Dumper;

my $font_hight=8;
my $count=0;
my $asc=0;
print STDOUT "#include <avr/pgmspace.h>\n";
print STDOUT "prog_char Font[][".$font_hight."] PROGMEM =\n{\n";

while(defined(my $char=getc(STDIN))) {
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
		print STDOUT " */\n";
		print STDOUT "\t\t0b00000000,\n";
		print STDOUT "\t\t0b00000000,\n";
		print STDOUT "\t\t0b00000000,\n";
		print STDOUT "\t\t0b00000000,\n";
		print STDOUT "\t\t0b00000000,\n";
	}

	$count++;
	my $byte=ord $char;
	print STDOUT "\t\t0b0";
	for(my $i=0;$i<7;$i++,$byte <<=1) {
		print STDOUT $byte & 0x80 ? "1" : "0";
	}
	if(($count % $font_hight)==0) {
		print STDOUT ",\n\t\t0b00000000\n\t}";
	} else {
		print STDOUT ",\n";
	}
}
print STDOUT "\n};\n";
