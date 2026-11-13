
### Class GateMOS: Create a MorphOS gatestub file #############################

BEGIN {
    package GateMOS;
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

	print "#include <emul/emulregs.h>\n";
	print "\n";
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	print "$prototype->{return}\n";
	print "$gateprefix$prototype->{funcname}(void)";
	
	if ($self->{PROTO}) {
	    print ";\n";
	}
	else {
	    print "\n";
	    print "{\n";
	}
    }

    sub function_arg {
	my $self      = shift;

	if (!$self->{PROTO}) {
	    my %params    = @_;
	    my $prototype = $params{'prototype'};
	    my $argtype   = $params{'argtype'};
	    my $argname   = $params{'argname'};
	    my $argreg    = $params{'argreg'};
	    my $argnum    = $params{'argnum'};
	    my $sfd       = $self->{SFD};

	    print "  $prototype->{___args}[$argnum] = ($argtype) REG_" .
		(uc $argreg) . ";\n";
	}
    }
    
    sub function_end {
	my $self      = shift;

	if (!$self->{PROTO}) {
	    my %params    = @_;
	    my $prototype = $params{'prototype'};
	    my $sfd       = $self->{SFD};

	    if ($libarg ne 'none' && !$prototype->{nb}) {
		print "  $sfd->{basetype} _base = ($sfd->{basetype}) ".
		    "REG_A6;\n";
	    }
	    
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
