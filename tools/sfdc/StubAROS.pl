
### Class StubAROS: Create an AROS stub file ##################################

BEGIN {
    package StubAROS;
    use vars qw(@ISA);
    @ISA = qw( Stub );

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

	if ($prototype->{type} eq 'function') {
	    print "\n";
	    print "{\n";

	    if (!$prototype->{nb}) {
		print "  BASE_EXT_DECL\n";
	    }

	    if (!$prototype->{nr}) {
		print "  $prototype->{return} _res = ($prototype->{return}) ";
	    }
	    else {
		print "  ";
	    }

	    printf "AROS_LC%d%s($prototype->{return}, $prototype->{funcname},\n",
	    $prototype->{numargs}, $prototype->{nb} ? "I" : "";
	}
	else {
	    $self->SUPER::function_start (@_);
	}
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

	if ($$prototype{'type'} eq 'function') {
	    print "    AROS_LCA($argtype, $argname, " . (uc $argreg) . "),\n";
	}
	else {
	    $self->SUPER::function_arg (@_);
	}
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	if ($$prototype{'type'} eq 'function') {
	    if ($prototype->{nb}) {
		my $bt = "/* bt */";
		my $bn = "/* bn */";

		for my $i (0 .. $#{$prototype->{regs}}) {
		    if ($prototype->{regs}[$i] eq 'a6') {
			$bt = $prototype->{argtypes}[$i];
			$bn  =$prototype->{___argnames}[$i];
			last;
		    }
		}
		
		printf "    $bt, $bn, %d, $sfd->{Basename});\n",
		$prototype->{bias} / 6;
	    }
	    else {
		printf "    $sfd->{basetype}, BASE_NAME, %d, $sfd->{Basename});\n",
		$prototype->{bias} / 6;
	    }

	    if (!$prototype->{nr}) {
		print "  return _res;\n";
	    }
	    
	    print "};\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
