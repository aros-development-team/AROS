
### Class MacroLP: Create a LP-style macro file ###############################

BEGIN {
    package MacroLP;
    use vars qw(@ISA);
    @ISA = qw( Macro );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );	
	bless ($self, $class);
	
	$self->{macros_h} = "inline/macros.h";
	
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	$self->SUPER::header (@_);
	
	print "#ifndef __INLINE_MACROS_H\n";
	print "#include <$self->{macros_h}>\n";
	print "#endif /* !__INLINE_MACROS_H */\n";
	print "\n";

	if ($$sfd{'base'} ne '') {
	    print "#ifndef $self->{BASE}\n";
	    print "#define $self->{BASE} $$sfd{'base'}\n";
	    print "#endif /* !$self->{BASE} */\n";
	    print "\n";
	}
    }
    
    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} eq 'function') {
	    
	    $self->{FUNCARGTYPE} = '';
	    for my $argtype (@{$$prototype{'argtypes'}}) {
		if ($argtype =~ /\(\*\)/) {
		    $self->{FUNCARGTYPE} = $argtype;
		    last;
		}
	    }
	    
	    printf "	LP%d%s%s%s(0x%x, ", $$prototype{'numargs'},
	    $prototype->{nr} ? "NR" : "",
	    $prototype->{nb} ? "NB" : "",
	    $self->{FUNCARGTYPE} ne '' ? "FP" : "",
            $$prototype{'bias'};

	    if (!$prototype->{nr}) {
		print "$$prototype{'return'}, ";
	    }

	    print "$$prototype{'funcname'}";
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
	    if ($argtype =~ /\(\*\)/) {
		print ", __fpt, $argname, $argreg";
	    }
	    else {
		print ", $argtype, $argname, $argreg";
	    }
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
	    if (!$prototype->{nb}) {
		print ", ,(___base)\\\n";
	    }

	    if ($self->{FUNCARGTYPE} ne '') {
		my $fa = $self->{FUNCARGTYPE};

		$fa =~ s/\(\*\)/(*__fpt)/;
		
		print ", $fa";
	    }
	    
  	    print ")\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
