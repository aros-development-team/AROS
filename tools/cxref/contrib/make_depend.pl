#!/bin/sh
#
# C Cross Referencing & Documentation tool. Version 1.5.
#
# A Perl script to produce a neat output for makefile dependencies.
#
# Written by Andrew M. Bishop
#
# This file Copyright 1999,2003 Andrew M. Bishop
# It may be distributed under the GNU Public License, version 2, or
# any higher version.  See section COPYING of the GNU Public license
# for conditions under which this file may be redistributed.
#

exec perl -x $0 $*

exit 1

#!perl

# The C pre-processor arguments (-D, -I).
$cpp_args="";

# The files to check.
@cfiles=();
@hfiles=();

#
# Parse the command line arguments
#

if( $#ARGV==-1 )
  {
   print "Usage: make_depend filename1.h [filename2.h ...]\n";
   print "                   filename1.c filename2.c\n";
   print "                   [-Ddefine] [-Udefine] [-Iinclude]\n";
   exit 0;
  }

while ( $#ARGV >= 0 )
  {
   $_=$ARGV[0];
   if(-f $_)
     {
      push(@cfiles,$_) if(m/\.c$/);
      push(@hfiles,$_) if(m/\.h$/);
     }
   else
     {
      $cpp_args.=" '".$_."'";
     }
   shift;
  }

die "Error: no source files specified\n" if($#cfiles==-1);
die "Error: no header files specified\n" if($#hfiles==-1);

#
# The main program
#

@cfiles=sort(@cfiles);

$longest=0;
foreach $cfile (@cfiles)
  {
   $longest=length($cfile) if(length($cfile)>$longest);
  }

foreach $cfile (@cfiles)
  {
   %inc_headers=&GetFileHeaders($cfile);

   $ofile = $cfile;
   $ofile =~ s/\.c/.o/;

   printf("%-".$longest."s : %-".$longest."s",$ofile,$cfile);

   foreach $hfile (@hfiles)
     {
      if($inc_headers{$hfile})
        {
         print " $hfile";
        }
      else
        {
         print " "." "x(length($hfile));
        }
     }

   print "\n";
  }


#
# Get the included headers from an existing file.
#

sub GetFileHeaders
{
 local($file)=@_;
 local(%headers)=();
 local($depends,$header)=("");

 # Parse the file

 open(IN,"gcc -MM $file $cpp_args|") || die "Cannot run 'gcc -MM $file $cpp_args'\n";

 while(<IN>)
     {
      $depends.=$_;
     }

 close(IN);

 $depends =~ s/\\?\n//g;

 foreach $header (split(/ +/,$depends))
   {
    $headers{$header}=1;
   }

 return(%headers);
}
