
### Class FD: Create an old-style FD file #####################################

BEGIN {
    package FD;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}     = $params{'sfd'};
	$self->{BIAS}    = -1;
	$self->{PRIVATE} = -1;
	$self->{VERSION} = 1;
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "* \"$$sfd{'libname'}\"\n";
	print "* Automatically generated FD! Do not edit!\n";
	print "##base _$$sfd{'base'}\n";
	$self->{BIAS}    = -1;
	$self->{PRIVATE} = -1;
	$self->{VERSION} = 1;
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function' ||
	    $prototype->{type} eq 'cfunction') {
	    if ($self->{BIAS} != $$prototype{'bias'}) {
		$self->{BIAS} = $$prototype{'bias'};
		print "##bias $self->{BIAS}\n";
	    }

	    if ($self->{PRIVATE} != $$prototype{'private'}) {
		$self->{PRIVATE} = $$prototype{'private'};
		print $self->{PRIVATE} == 1 ? "##private\n" : "##public\n";
	    }

	    if ($self->{VERSION} != $$prototype{'version'}) {
		$self->{VERSION} = $$prototype{'version'};

		print "*--- functions in V$self->{VERSION} or higher ---\n";
	    }

	    if ($$prototype{'comment'} ne '') {
		my $comment = $$prototype{'comment'};

		$comment =~ s/^/\*/m;
		
		print "$comment\n";
	    }
	    
	    print "$$prototype{'funcname'}(";
	    print join (',', @{$$prototype{'argnames'}});
	    print ")(";

	    if ($prototype->{type} eq 'function') {
		print join (',', @{$$prototype{'regs'}});
	    }
	    elsif ($prototype->{type} eq 'cfunction') {
		print "base," unless $prototype->{nb};
		print "$prototype->{subtype}";
	    }
	    else {
		die;
	    }
	    
	    print ")\n";
	
	    $self->{BIAS} += 6;
	}
    }
    
    sub footer {
	my $self = shift;

	print "##end\n";
    }
}
