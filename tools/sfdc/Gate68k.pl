
### Class Gate68k: Create a AmigaOS gatestub file #############################

BEGIN {
    package Gate68k;
    use vars qw(@ISA);
    @ISA = qw( Gate );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	print "$prototype->{return}\n";
	print "$gateprefix$prototype->{funcname}(";
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

	if ($argnum != 0) {
	    print ",\n";
	}

	print "	$prototype->{___args}[$argnum] __asm(\"$argreg\")";
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($libarg ne 'none' && !$prototype->{nb}) {
	    if ($prototype->{numargs} > 0 ) {
		print ",\n";
	    }

	    print "	$sfd->{basetype} _base __asm(\"a6\")";
	}
	elsif ($prototype->{numargs} == 0) {
	    print "void";
	}

	if ($self->{PROTO}) {
	    print ");\n";
	}
	else {
	    print ")\n";
	    print "{\n";

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
	    print "}\n";
	}
    }
}
