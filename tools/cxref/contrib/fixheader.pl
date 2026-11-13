#!/bin/sh
#
# C Cross Referencing & Documentation tool. Version 1.5.
#
# A Perl script to determine the required headers for source files.
#
# Written by Andrew M. Bishop
#
# This file Copyright 1999 Andrew M. Bishop
# It may be distributed under the GNU Public License, version 2, or
# any higher version.  See section COPYING of the GNU Public license
# for conditions under which this file may be redistributed.
#

exec perl -x $0 $*

exit 1

#!perl

# The verbose flag
$verbose=0;

# The output flag
$output=0;

# The compiler program.
$cc="gcc";
$cflags="-Wall -c";

# The cxref options.
$cxref_root=".";
$cxref_name="cxref";
$cxref_output=".";

# The C pre-processor arguments (-D, -I).
$cpp_args="";

# The files to check.
@files=();

#
# Parse the command line arguments
#

if( $#ARGV==-1 )
  {
   print "Usage: fixheader filename [filename ...] [-v] [-o]\n";
   print "                 [-Odirname] [-Nbasename] [-Rdirname]\n";
   print "                 [-Ddefine] [-Udefine] [-Iinclude]\n";
   print "\n";
   print "-v    Output verbose information during the processing.\n";
   print "-o    Output a modified source file after testing.\n";
   exit 0;
  }

while ( $#ARGV >= 0 )
  {
   $_=$ARGV[0];
  switch:
     {
      $_ eq '-v'    && do { $verbose=1; last switch;};
      $_ eq '-o'    && do { $output=1; last switch;};
      m/^-R/        && die "The -R option is not implemented\n";
      m/^-N/        && do { if($_ eq "-N") {shift; $cxref_name=$ARGV[0];} else {$cxref_name=substr($_,2);} last switch;};
      m/^-O/        && do { if($_ eq "-O") {shift; $cxref_output=$ARGV[0];} else {$cxref_output=substr($_,2);} last switch;};
      -f $_         && do { push(@files,$_); last switch;};

      $cpp_args.=" '".$_."'";
     }
   shift;
  }

#
# The main program
#

foreach $file (@files)
  {
   # Initialise the headers

   @cur_headers=&GetFileHeaders($file);

   @all_headers=@cur_headers;

   %use_headers=();

   if($file =~ m/\.h$/)
       {
        @cxref_headers=&GetCxrefHeaders();

        foreach $h (@cxref_headers)
            {
             next if($file eq substr($h,1,length($h)-2));
             foreach $hh (@all_headers)
                 {
                  $h="" if($hh eq $h);
                 }

             @all_headers=(@all_headers,$h) if($h =~ m/^\"/);
             @all_headers=($h,@all_headers) if($h =~ m/^\</);
            }
       }

   foreach $header (@all_headers)
       {
        $use_headers{$header}=3;
       }

   foreach $header (@cur_headers)
       {
        $use_headers{$header}=1;
       }

   # Test the file as it is now

   print "\nTesting file $file\n" if($verbose);

   print "\nWe need to know how well it works without changing it\n" if($verbose);
   $current=&TryTheseHeaders($file,"unchanged file",());

   if($current>=0)
       {
        @all_headers=@cur_headers;
       }

   # Try all of the headers

   if($file =~ m/\.h$/)
       {
        print "\nThis is a header file so we should try all possible headers\n" if($verbose);
        $all=&TryTheseHeaders($file,"all headers",@all_headers);
       }
   else
       {
        $all=$current;
       }

   # Try removing headers

   print "\nNow we try to remove unneeded headers\n" if($verbose);

  tryagain:

   ($remove,@remain_headers)=&TryRemovingHeaders($file,$all,@all_headers);

   if(($all<0 && $all!=$remove) || $remove<$all)
       {
        $all=$remove;
        @all_headers=@remain_headers;
        print "\nWe need to try again now we have removed some problem headers\n" if($verbose);
        goto tryagain;
       }

   # Print out a summary

   print "\nSummary for $file\n\n";

   $added="";
   foreach $header (@all_headers)
       {
        print "    Removed: \#include $header\n" if($use_headers{$header}==0);
        print "    Kept   : \#include $header\n" if($use_headers{$header}==1);
        print "    Added  : \#include $header\n" if($use_headers{$header}==3);
        $added.=" $header" if($use_headers{$header}==3);
       }

   &FixupFile($file,$added) if($output);

   print "\n";
  }


#
# Get the included headers from an existing file.
#

sub GetFileHeaders
{
 local($file)=@_;
 local(@headers)=();

 # Parse the file

 open(IN,"<$file") || die "Cannot open the source file '$file'\n";

 while(<IN>)
     {
      push(@headers,$1) if(m/^[ \t]*\#include ([<\"][^>\"]+[>\"])/);
     }

 close(IN);

 return(@headers);
}


#
# Get the headers from the cxref database
#

sub GetCxrefHeaders
{
 local(@headers)=();
 local(%headers)=();

 open(INC,"<$cxref_output/$cxref_name.include") || die "Cannot open the cxref database file '$cxref_output/$cxref_name.include'\n";

 while(<INC>)
     {
      chop;
      local($file,@heads)=split(" +");

      foreach $h (@heads)
          {
           next if($headers{$h});
           $headers{$h}=1;

           if($h =~ m/^%/)
               {
                push(@headers,"\"".substr($h,1)."\"");
               }
           else
               {
                push(@headers,"<".$h.">");
               }
          }
     }

 close(INC);

 return(@headers);
}


#
# Try removing them one at a time.
#

sub TryRemovingHeaders
{
 local($file,$curr,@headers)=@_;
 local($best)=$curr;
 local(@best_headers)=();

 foreach $header (reverse(@headers))
     {
      next if($use_headers{$header}==0 || $use_headers{$header}==2);

      @try_headers=();

      foreach $h (@headers)
          {
           next if($use_headers{$h}==0 || $use_headers{$h}==2);
           push(@try_headers,$h) if($h ne $header);
          }

      $use_head=$use_headers{$header};
      $use_headers{$header}=-1;
      $try=&TryTheseHeaders($file,"to remove $header",@try_headers);
      $use_headers{$header}=$use_head;

      if(($curr>=0 && $try>=0 && $try<=$curr) || ($curr<0 && $try>=0))
          {
           $use_headers{$header}--;
          }

      if(($best>=0 && $try>=0 && $try<=$best) || ($best<0 && $try>$best))
          {
           $best=$try;
           @best_headers=@try_headers;
          }
     }

 return($best,@best_headers);
}


#
# Try the specified headers
#

sub TryTheseHeaders
{
 local($file,$what,@these)=@_;

 print "  Trying $what .." if($verbose);
 $result=&TryCompile($file,@these);
 print " ($result)" if($verbose);

 if($result>=0)
     {
      print " OK\n" if($verbose);
     }
 else
     {
      print " Failed\n" if($verbose);
     }

 return $result;
}


#
# Try compiling the file and see if it works.
#

sub TryCompile
{
 local($file,@headers)=@_;
 $length=0;

# Modify the file

 $tmpfile=$file;
 $tmpfile =~ s/\.([ch])/.cxref.$1.c/;

 die "Cannot create temporary filename from name '$file'\n" if($file eq $tmpfile);

 open(IN,"<$file") || die "Cannot open the source file '$file'\n";
 open(OUT,">$tmpfile") || die "Cannot create the temporary file '$tmpfile'\n";

 foreach $header (@headers)
     {
      print OUT "#include $header\n" if($use_headers{$header}==3);
     }

 while(<IN>)
     {
      next if(m/^[ \t]*\#include ([<\"][^>\"]+[>\"])/ && $use_headers{$1}!=1);
      print OUT;
     }

 close(IN);
 close(OUT);

# Test the compilation

 $result=system "$cc $cflags $tmpfile -o /dev/null $cpp_args > $file.cxref-result 2>&1";

 chop($length=`wc -l $file.cxref-result | awk '{print \$1}'`);

 unlink "$tmpfile";
 unlink "$file.cxref-result";

 $result=($result & 0xffff);

 if($result)
     {$length=-1-$length;}

 return($length);
}


#
# Fixup the headers in the file.
#

sub FixupFile
{
 local($file,$added)=@_;

# Modify the file

 $newfile=$file;
 $newfile =~ s/\.([ch])/.cxref.$1/;

 die "Cannot create temporary filename from name '$file'\n" if($file eq $newfile);

 open(IN,"<$file") || die "Cannot open the source file '$file'\n";
 open(OUT,">$newfile") || die "Cannot create the output file '$newfile'\n";

 if($added)
     {
      (@added)=split(" ",$added);

      foreach $h (@added)
          {
           printf OUT "#include %-30s /* Added by cxref */\n",$h;
          }

      print OUT "\n";
     }

 while(<IN>)
     {
      if(m/^[ \t]*\#include ([<\"][^>\"]+[>\"])(.*)$/)
          {
           printf OUT "/* #include %-30s* Removed by cxref */ %s\n",$1,$2 if($use_headers{$1}==0);
           print OUT $_ if($use_headers{$1}==1);
          }
      else
          {
           print OUT;
          }
     }

 close(IN);
 close(OUT);
}
