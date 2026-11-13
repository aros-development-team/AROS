#! /usr/bin/perl

# autodoc.pl - an autodoc-replacement
#
# Copyright (C) 2000, 2001 Joerg Dietrich
#
# This is the AROS-version of autodoc.pl.
# It is distributed under the AROS Public License.
# But I reserve the right to distribute
# my own version under other licenses.

# This is a quick and dirty hack. It is just for internal use.
# But feel free to improve it.

# read in all commandline-specified files
# into one big array
for(@ARGV)
{
 if(open(TMPFILE, "<".$_)) 
 {
  while(<TMPFILE>)
  {
   push(@Zeilen, $_);
  }
  close(TMPFILE);
 }
}

# count rows
$nZeilen=0;
for(@Zeilen)
{
 $nZeilen++;
}

# extract the autodocs
for($i=0; $i<$nZeilen; $i++)
{
 # scans for /******
 if($Zeilen[$i] =~ /\/\*{6}/)
 {
  # function-name as key of a hash-entry
  $Key=substr($Zeilen[$i], 8, rindex($Zeilen[$i], " ")-8);

  # Now put all following rows into the data of the hash-entry
  $Value="";
  while(1)
  {
   $i++;
   if($Zeilen[$i] =~ /\*{6}/)
   {
    last;
   }
   
   $Value=$Value . substr($Zeilen[$i], 1);
  }
  
  $Data{$Key}=$Value;
 }
}

#sort the keys alphabeticaly
@Keys=keys(%Data);
@KeysSorted=sort(@Keys);

#TOC
print("TABLE OF CONTENTS\n\n");

for(@KeysSorted)
{
 print($_, "\n");
}

# print the functions
for(@KeysSorted)
{
 $Space="";
 
 for($i=0; $i<(77-2*length($_)); $i++)
 {
  $Space=$Space." ";
 }

 print($_, $Space, $_, "\n");
 
 print($Data{$_});
}


