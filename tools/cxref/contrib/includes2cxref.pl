#!/bin/sh
#
# C Cross Referencing & Documentation tool. Version 1.5b.
#
# A Perl script to determine the headers to process from the cxref.include file.
#
# Written by Andrew M. Bishop
#
# This file Copyright 1999 Andrew M. Bishop
# It may be distributed under the GNU Public License, version 2, or
# any higher version.  See section COPYING of the GNU Public license
# for conditions under which this file may be redistributed.
#

if [ ! -f "$1" ]; then
   echo "Usage: $0 cxref.include"
   exit 1
fi

exec perl -x $0 $1

exit 1

#!perl

$|=1;

open(INCLUDE,"<$ARGV[0]") || die "Cannot open $ARGV[0]\n";

@files=();

while(<INCLUDE>)
  {
   ($cfile,@hfiles)=split(/[ \n]+/);

   foreach $hfile (@hfiles)
       {
        push(@files,substr($hfile,1)) if ($hfile =~ m/^%/);
       }
  }

close(INCLUDE);

$lastfile='';
@files=sort(@files);

foreach $file (@files)
  {
   if($file ne $lastfile)
       {
        print "cxref $file\n";
        `cxref $file`;
       }
   $lastfile=$file;
  }
