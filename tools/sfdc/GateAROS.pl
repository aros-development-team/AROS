
### Class GateAROS: Create an AROS gatestub file ##############################

BEGIN {
    package GateAROS;
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
	
	$self->SUPER::header (@_);

	print "#include <aros/libcall.h>\n";
	print "\n";
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

	if ($self->{PROTO}) {
	    printf "AROS_LD%d%s(", $prototype->{numargs}, $nb ? "I" : "";
	}
	else {
	    printf "AROS_LH%d%s(", $prototype->{numargs}, $nb ? "I" : "";
	}
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

	if ($self->{PROTO}) {
	    print "	AROS_LDA($argtype, $argname, " . (uc $argreg) . "),\n";
	}
	else {
	    print "	AROS_LHA($argtype, $argname, " . (uc $argreg) . "),\n";
	}	    
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
	    $bn = "_base";
	}

	printf "	$bt, $bn, %d, $sfd->{Basename})",
	$prototype->{bias} / 6;

	if ($self->{PROTO}) {
	    print ";\n";
	    print "#define $gateprefix$prototype->{funcname} " .
		"AROS_SLIB_ENTRY(" .
		"$gateprefix$prototype->{funcname},$sfd->{Basename})\n";
	}
	else {
	    print "\n";
	    print "{\n";
	    print "  AROS_LIBFUNC_INIT\n";
	    print "  return $libprefix$prototype->{funcname}(";

	    if ($libarg eq 'first' && !$prototype->{nb}) {
		print "_base";
		print $prototype->{numargs} > 0 ? ", " : "";
	    }

	    print join (', ', @{$prototype->{___argnames}});
	
	    if ($libarg eq 'last' && !$prototype->{nb}) {
		print $prototype->{numargs} > 0 ? ", " : "";
		print "_base";
	    }
	
	    print ");\n";
	    print "  AROS_LIBFUNC_EXIT\n";
	    print "}\n";
	}
    }
}
