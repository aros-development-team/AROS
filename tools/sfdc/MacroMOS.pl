
### Class MacroMOS: Implements MorphOS-only features for macro files ##########

BEGIN {
    package MacroMOS;
    use vars qw(@ISA);
    @ISA = qw (MacroLP);

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	
	$self->{macros_h} = "ppcinline/macros.h";
	
	return $self;
    }
   
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	
	if ($$prototype{'type'} eq 'function') {

	    if (!$prototype->{nb}) {
		print ",\\\n	, (___base)";
	    }

	    if ($self->{FUNCARGTYPE} ne '') {
		my $fa = $self->{FUNCARGTYPE};

		$fa =~ s/\(\*\)/(*__fpt)/;
		
		print ", $fa";
	    }
	    
	    print ", IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
