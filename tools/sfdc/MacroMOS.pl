
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

	    print ", IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
