
### Class LVO: Create an assembler LVO include file ###########################

BEGIN {
    package LVO;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD} = $params{'sfd'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "* Automatically generated header! Do not edit!\n";
	print "	IFND	LVO_$sfd->{BASENAME}_LIB_I\n";
	print "LVO_$sfd->{BASENAME}_LIB_I	SET	1\n";
	print "\n";
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};

	# Don't process private functions
	if ($prototype->{private}) {
	    return;
	}

	if ($prototype->{type} eq 'function') {
	    print "_LVO$prototype->{funcname}	EQU	-$prototype->{bias}\n";
	}
    }
    
    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "	ENDC	* LVO_$sfd->{BASENAME}_LIB_I\n";
    }
}
