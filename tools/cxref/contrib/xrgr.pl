#! /usr/bin/perl -w

use strict;
use vars qw/$VERSION/;
BEGIN { $VERSION = '0.91' }

use Getopt::Long;
use Pod::Usage;

Getopt::Long::Configure ("bundling");
my %args;
GetOptions (\%args, 'h|?|help', 'V|version', 'd|debug', 
	'n|filename|file=s@',
	'o|output=s',
	'k|defined',
	'v|verbose',
	't|paper=s@',
	'm|distance=s',
	'l|label=s',
	'c|cluster=s@',
	'f|function|func=s@') or pod2usage(2);

pod2usage(-verbose => 2, -exitval => 1) if $args{'h'};

if ($args{'V'}) {
    print STDERR "xrgr.pl Version $VERSION\n";
    exit;
}

sub dumpargs {
    print "filenames " . join(' ', @{$args{'n'}}) . "\n" if $args{'n'};
    print "functions " . join(' ', @{$args{'f'}}) . "\n" if $args{'f'};
    print "cluster   " . join(' ', @{$args{'c'}}) . "\n" if $args{'c'};
    print "paper     $args{'t'}\n" if $args{'t'};
    print "verbose   $args{'v'}\n" if $args{'v'};
    print "defined   $args{'k'}\n" if $args{'k'};
    print "distance  $args{'m'}\n" if $args{'m'};
    print "output    $args{'o'}\n" if $args{'o'};
    print "label     $args{'l'}\n" if $args{'l'};
}


#&dumpargs();

my %paper_sizes = (
    'a4' => "page=\"8.26,11.69\"; rotate=0; size=\"7.75,11.0\";",
    'a4r' => "page=\"8.26,11.69\"; rotate=90;size=\"11.0,7.75\";",
    'usr' => "page=\"8.5,11\"; rotate = 90; size = \"10.5,8.0\";",
    'us' => "page=\"8.5,11\"; rotate=0; size=\"8.0,10.5\";" ,
    'a1' => "page=\"594mm,841mm\"; rotate=0; size=\"575mm,820mm\";",
    'a1r' => "page=\"594mm,841mm\"; rotate=90; size=\"820mm,575mm\";",
    'a3' => "page=\"297mm,420mm\"; rotate=0; size=\"280mm,550mm\";",
    'a3r' => "page=\"297mm,420mm\"; rotate=90; size=\"550mm,280mm\";",
    );

my %paper_ratios = (
    'multi' => "ratio=auto;",
    'single' => "ratio=fill;",
    'auto' => "ratio=auto;",
    'fill' => "ratio=fill;",
    'compress' => "ratio=compress;",
    );

my $paper_headers;
my $paper_size_set = 0;
my $paper_ratio_set = 0;

my ($papers, $pstring);

if ($args{'t'}) {
	foreach $papers (@{$args{'t'}}) {
		if (defined($pstring = $paper_sizes{$papers})) {
			$paper_size_set = 1;
		} elsif (defined($pstring = $paper_ratios{$papers})) {
			$paper_ratio_set = 1;
		} else {
			die "Invalid -t paper type $papers";
		}
		$paper_headers .= "  $pstring\n";
	}
}
if (!$paper_size_set) {
	$paper_headers .= "  $paper_sizes{'a4r'}\n";
}

if (!$paper_ratio_set) {
	$paper_headers .= "  $paper_ratios{'single'}\n";
}

sub isDigits {
	/\d+/ ? 1 : 0;
}

my $distance;
$distance = 'all' if (!defined($distance = $args{'m'}));
die "-m distance can only be a number or \'all\'" if (!($distance =~ /\d+/) && $distance ne 'all') ;

my $verbose;
$verbose = 0 unless (defined($verbose = $args{'v'}));


local (*OUT);
if ($args{'o'}) {
	open( OUT, ">$args{'o'}" ) or die ("Couldn't open output file $args{'o'}");
	select(OUT);
}

print "digraph call {\n";
print "  concentrate = true; remincross = true;\n";
print "  $paper_headers";
print "  fontname=\"helvetica\"\n";
print "  fontsize = 12\n";
print "  margin=\"0.25\"\n";
print "  ranksep=\"0.5\"\n";
print "  exact_ranksep=false\n";
print "  center = 1\n";
print "  label=\"$args{'l'}\"\n" if $args{'l'};

my $xfilename;
local (*SRC);

if ($#ARGV < 0)
{
	processlines(*STDIN, "stdin");
} else {

	foreach $xfilename (@ARGV)
	{
		print STDERR "processing $xfilename\n";
		if (!open(SRC, $xfilename)) {
			print STDERR "Failed to open $xfilename : $!\n";
			next;
		}
		processlines(*SRC, $xfilename);
		close(SRC);
	}
}


#Format: filename funcname scope [[%][&]funcname1] [[%][&]funcname2] ... 
#The function funcname in file filename calls or references functions 
#funcname1, funcname2 ... ; those with a % are local, with a & are references.
#
#Format: filename $ 0 [[%]&funcname1] [[%]&funcname2] ... 
#The file references functions funcname1, funcname2 ... ; those with a % are 
#local.

my %funcdef;
my %calls;
my %called;
my %tempcalled;
my %filedef;
my %simplefuncname;
my %globalmap;

# simplefuncname{simplefuncname} -> list of all functions with this name (could
#                            have multiple static definitions)
# funcdef{funcname} -> filename
# calls{funcname}   -> list of called functions
# called{funcname}  -> list of functions that call funcname directly
#
# filedef{filename} = list of functions defined in that file
# globalmap{globfun} = maps a global function name to a function-filename

sub processlines {
	my ($infile, $fname) = @_;
	my $lcount = 0;
	my $calling;
	my $simplename;
	while(<$infile>) {
                chomp;
		$lcount++;
		if (length($_) == 0) {
			next;
		}
		my @slist = split(/ /, $_);
		if (scalar(@slist) < 3) {
			die ("$fname has invalid line $lcount");
		}
		my ($filename) = shift(@slist);
		my ($funcname) = shift(@slist);
		my ($scope) = shift(@slist);
		if ($funcname eq '$') {
			next;	# ignore for now
			$funcname = 'BODY';
		}
		$simplename = $funcname;
		my $funcfilename = "$funcname" . "-$filename";
		if ($scope < 2) {
			$funcname .= "-$filename";
		} else {
			# global symbol
			$globalmap{$funcname} = $funcfilename;
		}

		push(@{$simplefuncname{$simplename}}, $funcfilename);

		$funcdef{$funcfilename} = $filename;
		push (@{$filedef{$filename}}, $funcfilename);
		foreach $calling (@slist) {
			my $islocal = 0;
			if ($calling =~ s/^%//) {
				$islocal = 1;
			}
			if ($calling =~ s/^\&//) {
				# a reference
			}
			if ($islocal) {
				$calling .= "-$filename";
			}
			push (@{$calls{$funcfilename}}, $calling);
			push (@{$tempcalled{$calling}}, $funcfilename);
		}
		#print "$filename should eq $funcdef{$funcname}\n";
		#print "function $funcname calling " . join(' ', @{$calls{$funcname}}) . "\n" if (defined($calls{$funcname}));
		#print "calls " . join(' ', @{$tempcalled{$calling}}) . "\n";
	}
}

sub fixfuncnames {
	# after reading in, the global names in called and calls must be fixed
	# up to point to the correct file
	my ($furef, $fukey, $fumap);
	foreach $fukey (keys(%tempcalled)) {
		my ($funcname, $filename) = split(/-/, $fukey);
		if (!$filename) {
			if (defined($fumap = $globalmap{$fukey})) {
				$called{$fumap} = $tempcalled{$fukey};
			} else {
				$called{$fukey} = $tempcalled{$fukey};
			}
		} else {
			$called{$fukey} = $tempcalled{$fukey};
		}
	}
	foreach $fukey (keys(%calls)) {
		foreach $furef (@{$calls{$fukey}}) {
			my ($funcname, $filename) = split(/-/, $furef);
			if (!$filename) {
				if (defined($fumap = $globalmap{$furef})) {
					$furef = $fumap;
				}
			}
		}
	}
}

fixfuncnames();

sub dumpxref {
	my ($firef, $furef, $ref, $fulist);
	foreach $firef (keys(%filedef)) {
		my $lref = $filedef{$firef};
		foreach $ref (@{$lref}) {
			my $funcdname = $ref;
			my $scope = 2;
			my $junk = '';
			($funcdname, $junk) = split(/\-/, $ref);
			if (defined($junk) && length($junk)) {
				$scope = 1;
			}
			if ($funcdname eq 'BODY') {
				$funcdname = '$';
				$scope = 0;
			}
			print "$firef $funcdname $scope";
			if (!defined($fulist = $calls{$ref})) {
				print "\n";
				next;
			}
			foreach $furef (@$fulist) { 
				my $funccname = $furef;
				$junk = '';
				$scope = 2;
				($funccname, $junk) = split(/\-/, $furef);
				if (defined($junk) && length($junk)) {
					$scope = 1;
					$funccname = "\%$funccname";
				}
				print " $funccname";
			}
			print "\n";
		}
	}
}

my %seendnfun;
my %seenupfun;
my %seenfun;

sub seefun {
	my ($fname) = @_;
	my ($funcname, $junk, $filename);
	if (!$seenfun{$fname}) {
		$seenfun{$fname} = 1;
		if (!$args{'k'} || defined($filename = $funcdef{$fname})) {
			# only interested in defined functions
			($funcname, $junk) = split(/\-/, $fname);
			if ($verbose) {
				print "\"$fname\" [label=\"$funcname\\n$filename\"];\n";
			} else {
				print "\"$fname\" [label=\"$funcname\"];\n";
			}
		}
	}
}

sub tracednfunction {
	my ($fname, $recdepth) = @_;
	my ($fulist, $funcname, $junk, $filename, $furef);
	if (!$seendnfun{$fname}) {
		$seendnfun{$fname} = 1;
		seefun($fname);
		if (($distance eq 'all' || $recdepth < $distance) &&
			(defined($fulist = $calls{$fname}))) {
			foreach $furef (@$fulist) { 
				if (!$args{'k'} || defined($filename = $funcdef{$furef})) {
					print "  \"$fname\" -> \"$furef\";\n";
					&tracednfunction($furef, $recdepth+1);
				}
			}
		}
	}
}

sub traceupfunction {
	my ($fname, $recdepth) = @_;
	my ($fulist, $funcname, $junk, $filename, $furef);
	if (!$seenupfun{$fname}) {
		$seenupfun{$fname} = 1;
		seefun($fname);
		if (($distance eq 'all' || $recdepth < $distance) &&
			(defined($fulist = $called{$fname}))) {
			foreach $furef (@$fulist) { 
				if (!$args{'k'} || defined($filename = $funcdef{$furef})) {
					print "  \"$furef\" -> \"$fname\";\n";
					&traceupfunction($furef, $recdepth+1);
				}
			}
		}
	}
}

sub dumpall {
	my ($firef, $furef, $fucref, $fulist, $filename );
	foreach $firef (keys(%filedef)) {
		my $lref = $filedef{$firef};
		foreach $furef (@{$lref}) {
			if ($seendnfun{$furef}) {
				next;
			}
			$seendnfun{$furef} = 1;
			my $funcdname;
			my $junk = '';
			if (!$args{'k'} || defined($filename = $funcdef{$furef})) {
				# only interested in defined functions
				if (!defined($fulist = $calls{$furef})) {
					next;
				}
				foreach $fucref (@$fulist) { 
					if (!$args{'k'} || defined($filename = $funcdef{$fucref})) {
						seefun($furef);
						seefun($fucref);
						print "  \"$furef\" -> \"$fucref\";\n";
					}
				}
			}
		}
	}
}
		
if ($args{'f'}) {
	# only interested in certain functions
	my ($fusref, $funame, $furef);
	foreach $funame (@{$args{'f'}}) {
		if (defined($fusref = $simplefuncname{$funame})) {
			foreach $furef (@$fusref) {
				tracednfunction($furef, 0);
				traceupfunction($furef, 0);
			}
		}
	}
} elsif ($args{'n'}) {
	# only interested in certain files
	my ($fusref, $filename, $furef);
	foreach $filename (@{$args{'n'}}) {
		if (defined($fusref = $filedef{$filename})) {
			foreach $furef (@$fusref) {
				tracednfunction($furef, 0);
				traceupfunction($furef, 0);
			}
		}
	}
} else {

	&dumpall();
}

sub printcluster {
	my ($cname) = @_;
	my $first_elt = 1;
	if (!defined(${filedef{$cname}})) {
		return;
	}
	my $lref = $filedef{$cname};
	my $furef;
	foreach $furef (@{$lref}) {
		if ($seenfun{$furef}) {
			if ($first_elt) {
				$first_elt = 0;
				print "subgraph \"cluster_$cname\" { label=\"$cname\"\n";
			}
			print "  \"$furef\";\n";
		}
		#my $funcdname;
		#my $junk = '';
		#($funcdname, $junk) = split(/\-/, $ref);
		#print "$funcdname;\n";
	}
	if (!$first_elt) {
		print "}\n";
	}
}

# now, output any cluster definitions
if ($args{'c'}) {
	my $cname;
	foreach $cname (@{$args{'c'}}) { 
		if ($cname eq 'all') {
			my $firef;
			foreach $firef (keys(%filedef)) {
				&printcluster($firef);
			}
			last;
		} else {
			&printcluster($cname);
		}
	}
}

print "}\n";

=head1 NAME

xrgr - cxref to graphviz processor

=head1 SYNOPSIS

B<xrgr> [B<-m> distance] [B<-c> cluster] [B<-n> filename] [B<-f> function] [filespec] [filespec...]

=head1 DESCRIPTION

Process entries produced by the cxref program from the cxref.function
file and produces a .dot file for use with the graphviz program 'dot'.

This can be used to produce a printable call tree graph diagram.

Graphviz can be obtained from http://www.research.att.com/sw/tools/graphviz/

Various options can be used to limit the view to a few functions or files.

=head1 OPTIONS

=over 5

=item B<-m> B<--distance> value

This controls the number of functions that will be printed out. This represents
the calling and called distance to and from the nominated function or
file. If the value B<1> was used, only the directly called functions,
and the functions that directly called the function or filename of interest
would be output.

Default value is B<all>, meaning B<all> possible called and calling functions 
are output.

=item B<-n> B<--filename> B<--file> filename

Specify the filename(s) of interest. Only functions contained in the
file(s) are to printed, together with any called and calling functions,
to a depth controlled by the B<-m> or B<--distance> switch.

Multiple B<-n> options can be used to specify multiple filenames.

=item B<-c> B<--cluster> filename

Output the functions in the nominated file(s) as belonging to a cluster.

The value B<all> can be used to place all functions in their respective
files as clusters.

Multiple B<-c> options can be used to specify multiple filenames as clusters.

=item B<-f> B<--function> B<--func> function_name

Specify the function(s) of interest. Only these functions 
are to printed, together with any called and calling functions,
to a depth controlled by the B<-m> or B<--distance> switch.

Multiple B<-f> options can be used to specify multiple functions.

=item B<-k> B<--defined>

Only functions that are defined in the group of files given to cxref
should be output. Otherwise all called
functions are included, e.g., stdio functions, etc. 

=item B<-t> B<--paper> papersize

Set the paper size and orientation (a4r default), and whether multi page
or single page (single default);

Values are B<a4>, B<a4r>, B<us>, B<usr>, B<a1>, B<a1r>, B<a3>, B<a3r> 
for page size and orientation.

B<single> for single page, B<multi> for multiple pages. You can
also use B<compress> to force the page to do a lot to fit, but you
probably wont be able to read the function names.

Use the B<-t> option twice if you wanted to specify a page size and 
multiple pages.

=item B<-v> B<--verbose>

Include the filename in the node when displaying the function name.

=item B<-l> B<--label> label

Specify the label to be printed with the graph.

=item B<-o> B<--output> filename

Specify the output filename rather than stdout.

=item B<-V> B<--version>

Version. Print out the current version of the script and exit.

=item B<-h> B<--help>

Help. Print out this help.

=back

=head1 EXAMPLES

C<xrgr.pl -f main -o graph.out cxref.function>

Produces a graphviz dot file with every function calling or called by
B<main>, either directly or indirectly.

C<xrgr.pl -c main.c -o graph.out cxref.function>

Produce a graphviz dot file for every function. Cluster all the
functions in the module main.c into their own box.

=head1 INSTALLATION

Copy this script to a suitable place, e.g. /usr/local/bin or ~/bin.

=head1 COPYRIGHT

Copyright (c) 2002 Jamie Honan.

This script is free software; you can redistribute it and/or modify it
under the same terms as Perl itself.

=head1 AUTHOR

     Jamie Honan
     jhonan@optushome.com.au

=cut

