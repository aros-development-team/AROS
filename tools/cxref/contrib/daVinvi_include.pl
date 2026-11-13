#!/bin/sh
#
# Converts the cxref output files into a format that can be read into
# the DaVinci graph drawing program.
#
# (c) 1999 Andrew M. Bishop
#

exec perl -x $0 $*

#!perl

die "Usage: $0 cxref.include\n" if($#ARGV==-1);

open(INCLUDE,"<$ARGV[0]") || die "Cannot open $ARGV[0]\n";

%sysinclude=();

print "[\n";

while(<INCLUDE>)
  {
   chop;
   ($file,@includes)=split(/ /);

   if($file =~ /\.c$/)
       {
        print "l(\"$file\",n(\"\",[a(\"OBJECT\",\"$file\"),a(\"_GO\",\"ellipse\")],\n";
       }
   else
       {
        print "l(\"$file\",n(\"\",[a(\"OBJECT\",\"$file\")],\n";
       }

   print "\t[\n";
   foreach $include (@includes)
       {
        if($include =~ /^%(.+)$/)
            {
             print "\te(\"\",[],r(\"$1\")),\n";
            }
        else
            {
             $sysincludes{$include}=1;
             print "\te(\"\",[a(\"EDGEPATTERN\",\"dotted\")],r(\"<$include>\")),\n";
            }
       }
   print "\t]))\n,\n";
  }

foreach $include (keys(%sysincludes))
  {
   print "l(\"<$include>\",n(\"\",[a(\"OBJECT\",\"$include\"),a(\"COLOR\",\"#808080\")],[]))\n,\n";
  }

print "]\n";

close(INCLUDE);
