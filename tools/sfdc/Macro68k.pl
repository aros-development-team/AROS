
### Class Macro68k: Implements m68k-only features for macro files #############

BEGIN {
    package Macro68k;
    use vars qw(@ISA);
    @ISA = qw( MacroLP );

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

	if ($$prototype{'type'} eq 'function') {

	    my $regs      = join(',', @{$$prototype{'regs'}});
	    my $argtypes  = join(',', @{$$prototype{'argtypes'}});
	    my $a4        = $regs =~ /a4/;
	    my $a5        = $regs =~ /a5/;
	    my $fp        = $argtypes =~ /\(\*\)/;

	    if ($a4 && $a5 && !$quiet) {
		print STDERR "$$prototype{'funcname'} uses both a4 and a5 " .
		    "for arguments. This is not going to work.\n";
	    }
	    
	    $self->{FUNCARGTYPE} = '';
	    for my $argtype (@{$$prototype{'argtypes'}}) {
		if ($argtype =~ /\(\*\)/) {
		    $self->{FUNCARGTYPE} = $argtype;
		    last;
		}
	    }
	
	    printf "	LP%d%s%s%s%s%s(0x%x, ", $$prototype{'numargs'},
	    $prototype->{nr} ? "NR" : "",
	    $prototype->{nb} ? "NB" : "",
	    $a4 ? "A4" : "", $a5 ? "A5" : "",
	    $self->{FUNCARGTYPE} ne '' ? "FP" : "",
	    $$prototype{'bias'};

	    if (!$prototype->{nr}) {
		print "$$prototype{'return'}, ";
	    }

	    print "$$prototype{'funcname'} ";
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
	    my $argtype   = $params{'argtype'};
	    my $argname   = $params{'argname'};
	    my $argreg    = $params{'argreg'};
	    
	    if ($argreg eq 'a4' || $argreg eq 'a5') {
		$argreg = 'd7';
	    }
	    
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
		print ",\\\n	, $self->{BASE}";
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
