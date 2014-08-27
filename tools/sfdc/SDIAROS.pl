
### Class SDIAROS: Create an AROS SDI stub file ##############################

BEGIN {
    package SDIAROS;
    use vars qw(@ISA);
    @ISA = qw( Gate );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};
	
	$self->SUPER::header (@_);

	print "#include <aros/libcall.h>\n";
	print "#include <SDI_lib.h>\n";
	print "\n";
    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'cfunction') {
	    print "#define $gateprefix$prototype->{funcname} " .
		"AROS_SLIB_ENTRY(" .
		"$gateprefix$prototype->{funcname},$sfd->{Basename},".
		($prototype->{bias}/6).")\n\n";
	}

	$self->SUPER::function (@_);
    }
	
    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	my $nb        = $prototype->{nb} || $libarg eq 'none';

	# AROS macros cannot handle function pointer arguments :-(

	for my $i (0 .. $prototype->{numargs} - 1) {
	    if ($prototype->{argtypes}[$i] =~ /\(\*\)/) {
		my $typedef  = $prototype->{argtypes}[$i];
		my $typename = "$sfd->{Basename}_$prototype->{funcname}_fp$i";

		$typedef =~ s/\(\*\)/(*_$typename)/;
		    
		print "typedef $typedef;\n";
	    }
	}

	printf "AROS_LH%d(", $prototype->{numargs};

	print "$prototype->{return}, $gateprefix$prototype->{funcname},\n";
    }

    sub function_arg {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $argtype   = $params{'argtype'};
	my $argname   = $params{'argname'};
	my $argreg    = $params{'argreg'};
	my $argnum    = $params{'argnum'};
	my $sfd       = $self->{SFD};

	if ($argtype =~ /\(\*\)/) {
	    $argtype = "_$sfd->{Basename}_$prototype->{funcname}_fp$argnum";
	}

	print "    AROS_LHA($argtype, $argname, " . (uc $argreg) . "),\n";
    }

    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	my $bt = "/* bt */";
	my $bn = "/* bn */";

	if ($prototype->{nb}) {
	    for my $i (0 .. $#{$prototype->{regs}}) {
		if ($prototype->{regs}[$i] eq 'a6') {
		    $bt = $prototype->{argtypes}[$i];
		    $bn  =$prototype->{___argnames}[$i];
		    last;
		}
	    }
	}
	else {
	    $bt = $sfd->{basetype};
	    $bn = "__BASE_OR_IFACE_VAR";
	}

	print "    $bt, $bn, 0, LIBSTUB\n)";

	print "\n";
	print "{\n";
	print "    AROS_LIBFUNC_INIT\n";

	if ($prototype->{numargs} > 0) {
	    print "    return CALL_LFUNC($libprefix$prototype->{funcname}, ";
	    print join (', ', @{$prototype->{___argnames}});
	}
	else {
	    print "    return CALL_LFUNC_NP($libprefix$prototype->{funcname}";
	}
    
	print ");\n";
	print "    AROS_LIBFUNC_EXIT\n";
	print "}\n";
    }
}
