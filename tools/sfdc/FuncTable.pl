
### Class FuncTable: Create a function table fragment #########################

BEGIN {
    package FuncTable;

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

	print "/* Automatically generated function table! Do not edit! */\n";
	print "\n";
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};

	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'cfunction') {
	    print "	$gateprefix$prototype->{funcname},\n";
	}
    }
    
    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
    }
}
