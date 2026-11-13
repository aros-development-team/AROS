
### Class Verify: Verify SFD info #################################################

BEGIN {
    package Verify;

    sub new {
	my $proto       = shift;
	my %params      = @_;
	my $class       = ref($proto) || $proto;
	my $self        = {};
	$self->{SFD}    = $params{'sfd'};
	$self->{CNT}    = 0;
	$self->{FUNCS}  = {};
	$self->{ERRORS} = 0;
	$self->{WARNS}  = 0;
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self      = shift;
	my $sfd       = $self->{SFD};

	print "Checking SFD for $$sfd{'libname'} ...";
	$self->{CNT} = 0;

	if ($#{@{$sfd->{typedefs}}} != -1) {
	    print "\nWarning: SFD uses nonstandard '==typedef' command.";
	    ++$self->{WARNS};
	}
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($self->{FUNCS}{$prototype->{funcname}}) {
	    if ($prototype->{private}) {
		print "\nWarning: Private function $prototype->{funcname}() ".
		    "is defined more than once!";
		++$self->{WARNS};
	    }
	    else {
		print "\nError: Public function $prototype->{funcname}() ".
		    "is defined more than once!";
		++$self->{ERRORS};
	    }
	}
	else {
	    $self->{FUNCS}{$prototype->{funcname}} = 1;
	}

	++$self->{CNT};
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	if ($self->{WARNS} != 0 || $self->{ERRORS} != 0) {
	    print "\n$self->{WARNS} warning(s), $self->{ERRORS} error(s); ";

	    die if $self->{ERRORS};
	}
	
	printf " $self->{CNT} function%s verified\n", $self->{CNT} == 1 ? "" : "s";
    }
}
