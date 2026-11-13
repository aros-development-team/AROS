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

open(FUNCTION,"<$ARGV[0]") || die "Cannot open $ARGV[0]\n";

%sysfunctions=();
%localfunctions=();
%function=();

while(<FUNCTION>)
  {
   chop;
   ($file,$function,$scope,@calls)=split(/ /);

   $localfunctions{$function}=1;

   $function{$function}="$file:$function";
  }

print "[\n";

close(FUNCTION);

open(FUNCTION,"<$ARGV[0]") || die "Cannot open $ARGV[0]\n";

while(<FUNCTION>)
  {
   chop;
   ($file,$function,$scope,@calls)=split(/ /);

   if($scope==1)
       {
        print "l(\"$file:$function\",n(\"\",[a(\"OBJECT\",\"$function\")],\n";
       }
   else
       {
        print "l(\"$file:$function\",n(\"\",[a(\"OBJECT\",\"$function\")],\n";
       }

   print "\t[\n";
   foreach $call (@calls)
       {
        next if($call =~ /\&/);

        if($call =~ /^%(.+)$/)
            {
             print "\te(\"\",[],r(\"$file:$1\")),\n";
            }
        elsif($function{$call})
            {
             print "\te(\"\",[],r(\"$function{$call}\")),\n";
            }
        else
            {
             print "\te(\"\",[],r(\"$call\")),\n";
             $sysfunctions{$call}=1;
            }
       }
   print "\t]))\n,\n";
  }

foreach $function (keys(%sysfunctions))
  {
   print "l(\"$function\",n(\"\",[a(\"OBJECT\",\"$function\"),a(\"COLOR\",\"#808080\")],[]))\n,\n";
  }

print "]\n";

close(FUNCTION);
