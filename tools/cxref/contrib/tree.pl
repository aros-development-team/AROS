#!/bin/sh
#
# $Header$
#
# Converts the cxref.function output file into a format that can display
# a simple tree structure
#
# Warning: This is my first perl program. It can probably be improved on !
#
# This file Copyright 2000 Ian A. Gilmour.
# (ian@igilmour.freeserve.co.uk / ian.gilmour.ffei.co.uk)
#
# It may be distributed under the GNU Public License, version 2, or
# any higher version.  See section COPYING of the GNU Public license
# for conditions under which this file may be redistributed.
#
exec perl -w -x $0 $*

#!perl

$level = 0;
$func = "main";
$max_level = 3;
$found = 0;

# generates the tree going down 
# i.e. displays which functions are called by the given function
#
sub find_func {
    my $func_reqd = $_[0];

    if ($level > $max_level) { 
	return;
    } else {
	$level++;
    }
    
    for ($z=0; $z < ($level-1)*3; $z++) { 
	print " "; 
    }
    print "+- ";

    if (defined ($functions{$func_reqd})) {
        print "$func_reqd() \t[$file{$func_reqd}]";
	if($scope{$func_reqd}==2) {
	    print " \t(static)";
	}
	if ($fshown{$func_reqd}) {
	    print " \t...referenced above\n";
	} else {
	    print "\n";
	    $fshown{$func_reqd} = 1;

	    for (my $y=0; $y <= $fcount{$func_reqd}; $y++) {
	        find_func ($fcalls{$func_reqd}[$y]);
	    }
	}
    } else {
        if (!defined ($fundefshown{$func_reqd})) {
	    print "$func_reqd() \t[undefined]\n";
	    $fundefshown{$func_reqd} = 1;
	} else {
	    print "$func_reqd() \t[undefined] \t...referenced above\n";
	}
    }
    $level--;
}


# generates the tree going up 
# i.e finds which functions call the specified function
#
sub find_called_func {
    my $func_reqd = $_[0];
    
    if ($level <= $max_level) { 
	return;
    } else {
	$level--;
    }
    
    foreach my $func (keys %functions) {
        if (defined ($fcount{$func})) {
	    for (my $y=0; $y <= $fcount{$func}; $y++) {
                if ($func_reqd eq $fcalls{$func}[$y]) {
		    if (!$found) {
		        $found = 1;
			print "+- $func_reqd()\n";
		    }

		    for (my $z=0; $z > (($level)*3); $z--) { 
                        print " "; 
                    }
                    print "+- ";
                    print "$functions{$func} \t[$file{$func}]";
                    if($scope{$func}==1) {
                        print " \t(static)";
                    }
                    if ($fshown{$func}) {
                        print " \t...referenced above\n";
                    } else {
                        print "\n";
                        $fshown{$func} = 1;
                        find_called_func ($functions{$func});
                    }
		}
	    }
	}
    }
    $level++;

    if (!$found) {
        print "$func_reqd() not referenced.\n";
    };
    return;
}

die "Usage: $0 cxref_function_file root_function max_level\n".
    "max_level +ve for functions called by specified function\n".
    "max_level -ve for functions that call specified function\n" if($#ARGV==-1);

if (defined ($ARGV[2])) {
    $max_level = $ARGV[2];
};

if (defined ($ARGV[1])) {
    $func = $ARGV[1];
};

open(FUNCTION,"<$ARGV[0]") || die "Cannot open $ARGV[0]\n";

# read all function names in once to set up database
#
while(<FUNCTION>)  {
    s/\%//g;
    s/\&//g;
    chop;

    ($file,$function,$scope,@calls)=split(/ /);

    if((defined ($scope)) && (($scope==2) || ($scope==1)))  {
	$scope{$function}="$scope";
	$functions{$function}="$function";
	$file{$function}="$file";
	if (defined (@calls)) {  
	    $fcalls{$function} = [@calls];
	    $fcount{$function} = $#calls;
	    $fshown{$function} = 0;
	} else {
	    $fcount{$function} = -1;
	}
      }
}

close(FUNCTION);

# now display reqd info....
#
if ($max_level >= 0) {
  print "Looking for functions that $func() calls...\n"; 
  find_func ($func);
} else {
  print "Looking for functions that call $func()...\n"; 
  find_called_func ($func);
}
