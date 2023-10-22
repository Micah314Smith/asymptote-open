#!/usr/bin/env perl
#####
# findsym.pl
# Andy Hammerlindl 2010/06/01
#
#  Extract static symbols used in builtin.cc and write code so that they are
#  translated only once when creating the symbol table.
#####

use strict;
use warnings;

my $outname = shift(@ARGV);
if (not $outname) {
    print STDERR "usage ./findsym.pl out_symbols.h file1.cc file2.cc ...\n";
    exit(1);
}

open(header, ">$outname") ||
    die("Couldn't open $outname for writing");

print header <<END;
/*****
 * This file is automatically generated by findsym.pl
 * Changes will be overwritten.
 *****/

// If the ADDSYMBOL macro is not already defined, define it with the default
// purpose of referring to an external pre-translated symbol, such that
// SYM(name) also refers to that symbol.
#ifndef ADDSYMBOL
    #define ADDSYMBOL(name) extern sym::symbol PRETRANSLATED_SYMBOL_##name
    #define SYM(name) PRETRANSLATED_SYMBOL_##name
#endif

END

sub add {
  print header "ADDSYMBOL(".$_[0].");\n";
}

my %symbols = ();

foreach my $inname (@ARGV) {
    open(infile, $inname) ||
        die("Couldn't open $inname");
    while (<infile>) {
        while (m/SYM\(([_A-Za-z][_A-Za-z0-9]*)\)/gx) {
            $symbols{ $1 } = 1;
        }
    }
}

foreach my $s (sort keys %symbols) {
    add($s);
}
