#!/usr/bin/perl
#
# Copyright © 2005 Mozilla Corporation
#
# Permission to use, copy, modify, distribute, and sell this software
# and its documentation for any purpose is hereby granted without
# fee, provided that the above copyright notice appear in all copies
# and that both that copyright notice and this permission notice
# appear in supporting documentation, and that the name of
# Mozilla Corporation not be used in advertising or publicity pertaining to
# distribution of the software without specific, written prior
# permission. Mozilla Corporation makes no representations about the
# suitability of this software for any purpose.  It is provided "as
# is" without express or implied warranty.
#
# MOZILLA CORPORTAION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS, IN NO EVENT SHALL MOZILLA CORPORATION BE LIABLE FOR ANY SPECIAL,
# INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
# IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# Author: Vladimir Vukicevic <vladimir@pobox.com>
#

use MIME::Base64;

##
## Takes all the *.log files in the current directory (or those provided
## on the command line) and spits out html to stdout that can be used to
## view all the test results at once.
##

# show reference images
$config_show_ref = $ENV{'CAIRO_TEST_SHOW_REF'} || 0;

# show fail images
$config_show_fail = $ENV{'CAIRO_TEST_SHOW_FAIL'} || 1;

# show all results, even passes
$config_show_all = $ENV{'CAIRO_TEST_SHOW_ALL'} || 0;

# include test results inline
$config_show_inline = $ENV{'CAIRO_TEST_SHOW_INLINE'} || 0;

# end of config options

$tests = {};
$teststats = {};
$logs = {};

if ($#ARGV >= 0) { @files = @ARGV; } else { @files = <*.log>; }

foreach (<@files>) {
  my $testname;
  my $out_path, $diff_path, $ref_path;
  my $fn = $_;
  (open LOG, $fn) || next;
  while (<LOG>) {
    if (/^OUTPUT: (.*)$/) {
      $out_path = $1;
      next;
    }
    if (/^DIFFERENCE: (.*)$/) {
      $diff_path = $1;
      next;
    }
    if (/^REFERENCE: (.*)$/) {
      $ref_path = $1;
      next;
    }
    next unless /^TEST: (.*) TARGET: (.*) FORMAT: (.*) OFFSET: (.*) SIMILAR: (.*) RESULT: ([A-Z]*).*$/;
    $testname = $1 if !defined($testname);
    $tests->{$1} = {} unless $tests->{$1};
    $tests->{$1}->{$2} = {} unless $tests->{$1}->{$2};
    $tests->{$1}->{$2}->{$3} = {} unless $tests->{$1}->{$2}->{$3};
    $tests->{$1}->{$2}->{$3}->{$4} = {} unless $tests->{$1}->{$2}->{$3}->{$4};
    $tests->{$1}->{$2}->{$3}->{$4}->{$5}->{'out'} = $out_path;
    $tests->{$1}->{$2}->{$3}->{$4}->{$5}->{'diff'} = $diff_path;
    $tests->{$1}->{$2}->{$3}->{$4}->{$5}->{'ref'} = $ref_path;
    $tests->{$1}->{$2}->{$3}->{$4}->{$5}->{'result'} = $6;

    $teststats->{$2} = {"PASS" => 0, "FAIL" => 0, "NEW" => 0, "XFAIL" => 0, "UNTESTED" => 0, "CRASHED" =>0}
      unless $teststats->{$2};
    ($teststats->{$2}->{$6})++;

    undef $out_path;
    undef $diff_path;
    undef $ref_path;
  }
  close LOG;

  (open LOG, $fn) || die "I could open it earlier, but I can't now: $!";
  {
    local $/;
    $logs->{$testname} = <LOG>;
  }
  close LOG;
}

my $targeth = {};
my $formath = {};
my $offseth = {};
my $similarh = {};

foreach my $testname (sort(keys %$tests)) {
  my $v0 = $tests->{$testname};
  foreach my $targetname (sort(keys %$v0)) {
    my $v1 = $v0->{$targetname};

    $targeth->{$targetname} = 1;
    foreach my $formatname (sort(keys %$v1)) {
      my $v2 = $v1->{$formatname};

      $formath->{$formatname} = 1;
      foreach my $offsetval (sort(keys %$v2)) {
        my $v3 = $v2->{$offsetval};

        $offseth->{$offsetval} = 1;
        foreach my $similarval (sort(keys %$v3)) {
          $similarh->{$similarval} = 1;
        }
      }
    }
  }
}

my @targets = sort(keys %$targeth);
my @formats = sort(keys %$formath);
my @offsets = sort(keys %$offseth);
my @similars = sort(keys %$similarh);

sub printl {
  print @_, "\n";
}

# convert file into a data URI
sub file_to_data {
  my ($ctype,$fname) = @_;
  my $fdata;
  open FILE, $fname || return "data:" . $ctype . ",";
  {
    local $/;
    $fdata = encode_base64(<FILE>);
  }
  close FILE;
  return "data:" . $ctype . ";base64," . $fdata;
}

# convert string into a data URI
sub string_to_data {
  my ($ctype,$str) = @_;
  $str =~ s/([^A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg;
  return "data:" . $ctype . "," . $str;
}

printl '<html><head>';
printl '<title>Cairo Test Results</title>';
printl '<style type="text/css">';
printl 'a img { border: solid 1px #FFF; }';
printl '.PASS { background-color: #0B0; min-width: 1em; }';
printl '.NEW { background-color: #B70; }';
printl '.FAIL { background-color: #B00; }';
printl '.XFAIL { background-color: #BB0; }';
printl '.UNTESTED { background-color: #555; }';
printl '.CRASHED { background-color: #F00; color: #FF0; }';
printl '.PASSstr { color: #0B0; }';
printl '.FAILstr { color: #D00; }';
printl '.XFAILstr { color: #BB0; }';
printl '.CRASHEDstr { color: #F00; }';
printl '.UNTESTEDstr { color: #555; }';
printl 'img { max-width: 15em; min-width: 3em; min-height: 3em; margin: 3px; }';
printl 'td { vertical-align: top; }';
printl '</style>';
printl '</script>';
printl '</head>';
printl '<body>';

printl '<table border="1">';
print '<tr><th>Test</th>';

foreach my $target (@targets) {
  print '<th>', $target, '</th>';
}
printl '</tr>';

print '<tr><td></td>';

foreach my $target (@targets) {
  print '<td>';
  print '<span class="PASSstr">', $teststats->{$target}->{"PASS"}, '</span>/';
  print '<span class="FAILstr">',
	$teststats->{$target}->{"FAIL"} +
	$teststats->{$target}->{"NEW"} +
	$teststats->{$target}->{"CRASHED"},
	'</span>/';
  print '<span class="XFAILstr">', $teststats->{$target}->{"XFAIL"}, '</span>/';
  print '<span class="UNTESTEDstr">', $teststats->{$target}->{"UNTESTED"}, '</span>';
  print '</td>';
}
printl '</tr>';

sub img_for {
  my ($fn, $withlink) = @_;

  return "" unless defined $fn;

  if ($config_show_inline) {
    $fn = file_to_data("image/png", $fn);
    # never return links, people can just right-click view image,
    # and we don't clutter the document
    return '<img src="' . $fn . '">';
  } else {
    if ($withlink) {
      return '<a href="' . $fn . '"><img src="' . $fn . '"></a>';
    } else {
      return '<img src="' . $fn . '">';
    }
  }
}

foreach my $test (sort(keys %$tests)) {
  foreach my $offset (@offsets) {
    foreach my $similar (@similars) {
      foreach my $format (@formats) {
        my $testline = "";

        foreach my $target (@targets) {
          my $tgtdata = $tests->{$test}->{$target};
          if ($tgtdata) {
            my $testres = $tgtdata->{$format}->{$offset}->{$similar};
            if ($testres) {
              my %testfiles;
              $testfiles{'out'} = $testres->{'out'};
              $testfiles{'diff'} = $testres->{'diff'};
              $testfiles{'ref'} = $testres->{'ref'};

              $testline .= "<td class=\"$testres->{'result'}\">";
              $teststats{$target}{$testres}++;
              if ($testres->{'result'} eq "PASS") {
                if ($config_show_all) {
                  $testline .= img_for($testfiles{'out'},1);
                }
              } elsif ($testres->{'result'} eq "FAIL") {
                if ($config_show_fail || $config_show_all) {
                  $testline .= img_for($testfiles{'out'},1);
                  $testline .= " ";
                  $testline .= img_for($testfiles{'diff'},1);
                  $testline .= " ";
                  $testline .= img_for($testfiles{'ref'},1);
                }
              } elsif ($testres->{'result'} eq "NEW") {
                if ($config_show_fail || $config_show_all) {
                  $testline .= img_for($testfiles{'new'},1);
                }
              } elsif ($testres->{'result'} eq "CRASHED") {
                 $testline .= "!!!CRASHED!!!";
              } elsif ($testres->{'result'} eq "XFAIL") {
                #nothing
                if ($config_show_all) {
                  $testline .= img_for($testfiles{'out'},1);
                  #$testline .= "<hr size=\"1\">";
                  $testline .= " ";
                  $testline .= img_for($testfiles{'diff'},1);
                  $testline .= " ";
                  $testline .= img_for($testfiles{'ref'},1);
                }
              } elsif ($testres->{'result'} eq "UNTESTED") {
                #nothing
              } else {
                $testline .= "UNSUPPORTED STATUS '$testres->{'result'}' (update make-html.pl)";
              }

              $testline .= "</td>";
            } else {
              $testline .= '<td></td>';
            }
          } else {
            $testline .= '<td></td>';
          }
        }
        print '<tr><td>';

        if ($config_show_inline) {
	  print "$test ($format/$offset) ";
	  print "(<a href=\"" . string_to_data("text/plain",$logs->{$test}) . "\">log</a>)";
        } else {
	  print $test, ' (', $format, '/', $offset, ($similar ? ' similar' : ''), ') ';
	  print "(<a href=\"$test.log\">log</a>)";
        }

        print '</td>';

        print $testline;

        print "</tr>\n";
      }
    }
  }
}

print "</table></body></html>\n";
