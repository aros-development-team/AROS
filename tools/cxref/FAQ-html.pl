#!/usr/bin/perl

#
# Copyright Andrew M. Bishop 1996.97,98,2001,03.
#
# Usage: FAQ-html.pl < FAQ > FAQ.html
#

$_=<STDIN>;
s/^ *//;
s/ *\n//;
$first=$_;

print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">\n";
print "<HTML>\n";
print "\n";
print "<HEAD>\n";
print "<TITLE>$first</TITLE>\n";
print "</HEAD>\n";
print "\n";
print "<BODY>\n";
print "\n";
print "<h1>$first</h1>\n";

$questions=0;
$answers=0;
$pre=0;
$blank=1;

while(<STDIN>)
  {
   chop;

   s/&/&amp;/g;
   s/</&lt;/g;
   s/>/&gt;/g;

   next if(m/^ *=+ *$/);
   next if ($_ eq "--------------------");
   $pre++,$blank=0,next if($pre==1 && $_ eq "");
   $blank=1,next        if($pre!=1 && $_ eq "");
   $pre++ if($pre);

   if ($_ eq "--------------------------------------------------------------------------------")
     {
      $pre=0,print "</pre>\n" if($pre);
      print "<hr>\n";
      $answers++              if( $answers);
      $questions=0,$answers=1 if( $questions);
      $questions=1            if(!$questions && !$answers);
     }
   elsif (m/^(Section [0-9]+)/)
     {
      $section = $1;
      $section =~ tr/ /-/;

      $pre=0,print "</pre>\n" if($pre);
      print "<p><b><a href=\"#$section\">$_</a></b>\n" if($questions);
      print "<h2><a name=\"$section\">$_</a></h2>\n"       if($answers);
     }
   elsif (m/^(Q [0-9]+.[0-9]+[a-z]*)/)
     {
      $question = $1;
      $question =~ tr/ /-/;

      $blank=0,$pre=0,print "</pre>\n" if($pre);
      print "<p><a href=\"#$question\">$_</a>\n"  if($questions);
      print "<h3><a name=\"$question\">$_</a></h3>\n" if($answers);
      $pre=1,print "<pre>\n" if($answers);
     }
   elsif (m/\((See Q [0-9]+.[0-9]+[a-z]*)\)/)
     {
      $question = substr($1,4);
      $question =~ tr/ /-/;
      $href=$1;

      s%$1%<a href="#$question">$href</a>% if($answers);
      print "$_\n";
     }
   elsif (m%(^|[^\'\"])(http://[A-Za-z0-9-_.]+/[/A-Za-z0-9-_.~]*)%)
     {
      $href=$2;

      s%$2%<a href="$href">$href</a>%;
      print "$_\n";
     }
   else
     {
      $blank=0,print "\n" if($blank);
      print "$_\n";
     }
  }

print "</BODY>\n";
print "\n";
print "</HTML>\n";
