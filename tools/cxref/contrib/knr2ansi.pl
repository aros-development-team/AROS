#!/bin/sh

exec perl -x $0 $*

#!perl

die "Usage: $0 <filename>\n" if($#ARGV==-1);

foreach $argv (@ARGV)
  {
   push(@files,$argv) if(-f $argv);
   push(@args,$argv) if(! -f $argv);
  }

$args=join(" ",@args);

foreach $file (@files)
  {
   open(CXREF,"cxref $args $file -raw |") || die "Cannot run cxref on $file\n";

   $function=0;
   $comment=0;
   $functype="";
   $funcname="";
   @funcargs=();

   @local=();
   @global=();

   while(<CXREF>)
       {
        if(m/^FUNCTION : ([a-z0-9A-Z_\$]+) \[([a-zA-Z]+)\]$/)
            {
             $function=1;
             $functype="static " if($2 eq "Local");
             $funcname=$1;
            }
        $comment=1 if($function && m/^<<<$/);
        $comment=0 if($function && m/^>>>$/);

        $functype.=substr($1,0,length($1)-length($funcname)-1) if($function && m/^Type: ([^<]+)( <|\n)/);
        push(@funcargs,$1) if($function && m/^Arguments: ([^<]+)( <|\n)/);

        if(!$comment && $function && (m/^$/ || m/^-+$/))
            {
             push(@funcargs,"void") if($#funcargs==-1);

             $f="$functype $funcname(".join(",",@funcargs).");";

             push(@local,$f)  if($functype =~ m/static/);
             push(@global,$f) if($functype !~ m/static/);

             $functype="";
             $funcname="";
             @funcargs=();
             $function=0;
            }
       }

   close(CXREF);

# Output the results

   print "\n /* local functions in $file */\n\n";

   foreach $f (@local)
       {
        print "$f\n";
       }

   print "\n /* global functions in $file */\n\n";

   foreach $f (@global)
       {
        print "$f\n";
       }

  }
