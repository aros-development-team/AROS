
### Class MacroAROS: Implements AROS macro files ##############################

BEGIN {
    package MacroAROS;
    use vars qw(@ISA);
    @ISA = qw( Macro );

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
	
	print "#ifndef AROS_LIBCALL_H\n";
	print "#include <aros/libcall.h>\n";
	print "#endif /* !AROS_LIBCALL_H */\n";
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
	my $numargs   = $$prototype{'numargs'};

	if ($prototype->{nb}) {
	    $numargs--;
	}
	if ($$prototype{'type'} eq 'function') {
	    printf "	AROS_LC%d%s(%s, %s, \\\n",
	    $numargs,
	    $$prototype{'return'} eq 'void'
		|| $$prototype{'return'} eq 'VOID' ? "NR" : "",
	    $$prototype{'return'}, $$prototype{'funcname'};
	}
	else {
	    $self->SUPER::function_start (@_);
	}
    }


    sub function_arg {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};

	if ($$prototype{'type'} eq 'function') {
	    my $argreg    = uc $params{'argreg'};
	    
	    if ($argreg ne 'A6') {
	        my $argtype   = $params{'argtype'};
	        my $argname   = $params{'argname'};
	    
	        print "	AROS_LCA($argtype, ($argname), $argreg), \\\n";
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
	    if( !$prototype->{nb}) {
		print "	$$sfd{'basetype'}, (___base), ";
	    }
	    else {
		my $bt = "/* bt */";
		my $bn = "/* bn */";

		for my $i (0 .. $#{$prototype->{regs}}) {
		    if ($prototype->{regs}[$i] eq 'a6') {
			$bt = $prototype->{argtypes}[$i];
			$bn  =$prototype->{___argnames}[$i];
			last;
		    }
		}
		
		print "	$bt, $bn, ";
	    }
	    
	    print $$prototype{'bias'} / 6;
	    print ", $sfd->{Basename})\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
