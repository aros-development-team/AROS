
### Class MacroAOS4: Create a AOS4-style macro file ###########################

BEGIN {
    package MacroAOS4;
    use vars qw(@ISA);
    @ISA = qw( Macro );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	my $sfd    = $self->{SFD};
	bless ($self, $class);
	
	$self->{CALLBASE} = "I$sfd->{BaseName}";
	return $self;
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function'  ||
	    $prototype->{type} eq 'cfunction' ||
	    $prototype->{type} eq 'varargs') {
	    printf "	(((struct $sfd->{BaseName}IFace *)(___base))->$prototype->{funcname})(";
	    #$prototype->{type} = 'cfunction';
	}
	else {
	    $self->SUPER::function_start (@_);
	}
    }

    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	
	if ($prototype->{type} eq 'function'  ||
	    $prototype->{type} eq 'cfunction' ||
	    $prototype->{type} eq 'varargs') {
	    print ")\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
