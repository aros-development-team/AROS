#! /usr/bin/perl

# autodoc.pl - an autodoc-replacement
#
# Copyright (C) 2000 Joerg Dietrich
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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


