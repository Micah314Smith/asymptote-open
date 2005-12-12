#!/usr/bin/env perl
######
# runtime.pl
# Tom Prince 2004/4/15
#
# Generates gen_run.cc from runtime.cc.
#
##### 

$stack = "Stack";

sub clean_type {
    for (@_) {
        s/\s//g;
    }
}

sub clean_params {
    for (@_) {
        s/\n//g;
    }
}

my %type_map;
sub read_types {
    my @types = split /\n/, shift;
    for (@types) {
        my ($type,$code) = 
            m|(\w*(?:\s*\*)?)
              \s*=>\s*
              (.*)
              |x;
        clean_type($type);
        $type_map{$type} = $code;
    }
}

sub asy_params {
    my @params = @_;
    for (@params) {
        my ($type, $name, $default) = 
            m|^\s*
              (\w*(?:\s*\*)?)
              \s*
              (\w*)(=*)|xs;
        clean_type($type);
        $_ = $type_map{$type} . ", \"" . $name . "\"" . ", " . ($default ? "true" : "false") ;
    }
    return @params;
}

sub c_params {
   my @params = @_;
   for (@params) {
       my ($type, $name, $default, $value) = 
            m|^\s*
              (\w*(?:\s*\*)?)
              \s*
              (\w*)(=*)(\w*)|xs;
       $_ = "  $type $name = vm::pop" . ($type =~ /^item$/ ? "" : "<$type>") . "($stack" . ($default ? "," . $value : "") . ");\n";
   }
   reverse @params;
}

$/ = "\f\n";

open STDIN, "<runtime.in";
open STDOUT, ">genrun.cc";

$header = <>;
$types = <>;
$header = $header . <>;

print "/***** Autogenerated from runtime.in; changes will be overwritten *****/\n\n";
print $header;
print "\nnamespace run {\n";

read_types($types);

my @builtins;
my $count = 0;
while (<>) {
  my ($comments,$type,$name,$cname,$params,$code) = 
    m|^((?:\s*//[^\n]*\n)*) # comment lines
      \s*
      (\w*(?:\s*\*)?)   # return type
      \s*
      ([^(:]*)\:*([^(]*) # function name
      \s*
      \(([\w\s*,=]*)\)  # parameters
      \s*
      \{(.*)}           # body
      |xs;

  # Unique C++ function name
  if(!$cname) {$cname="gen${count}";}
  
  clean_type($type);
  
  my @params = split m/,\s*/, $params;

  # Build addFunc call for asymptote
  if($name) {
  $name =~ s/operator\s*//;
  push @builtin, "  addFunc(ve, run::" . $cname 
      . ", " . $type_map{$type}
      . ", " . '"' . $name . '"'
      . ( @params ? ", " . join(", ",asy_params(@params))
                   : "" )
      . ");\n";
  }

  # Handle marshalling of values to/from stack
  $qualifier = ($type eq "item" ? "" : "<$type>");
  $code =~ s/\breturn ([^;]*);/{$stack->push$qualifier($1); return;}/g;
  $args = join("",c_params(@params));

  print $comments;
  if($name) {
    my $prototype=$type . " " . $name . "(" . $params . ");";
    clean_params($prototype);
    print "// $prototype\n";
  }
  print "void $cname(vm::stack *$stack)\n{\n$args$code}\n\n";
  
  ++$count;
}

print "} // namespace run\n";

print "\nnamespace trans {\n\n";
print "void gen_base_venv(venv &ve)\n{\n";
print @builtin;
print "}\n\n";
print "} // namespace trans\n";
