
### Class SDI: Create a generic SDI Stub file ################################

BEGIN {
    package SDI;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}      = $params{'sfd'};
	$self->{PROTO}    = $params{'proto'};
	$self->{LIBPROTO} = $params{'libproto'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	if ($self->{PROTO}) {
	    print "/* Automatically generated header! Do not edit! */\n";
	    print "\n";
	    print "#ifndef _$$sfd{'BASENAME'}_H\n";
	    print "#define _$$sfd{'BASENAME'}_H\n";
	}
	else {
	    print "/* Automatically generated SDI stubs! Do not edit! */\n";
	}
	print "\n";

	foreach my $inc (@{$$sfd{'includes'}}) {
	    print "#include $inc\n";
	}

	foreach my $td (@{$$sfd{'typedefs'}}) {
	    print "typedef $td;\n";
	}

	print "\n";
	print "#ifdef __cplusplus\n";
	print "extern \"C\" {\n";
	print "#endif /* __cplusplus */\n";
	print "\n";
    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function') {
	    $self->function_proto (prototype => $prototype);
	    $self->function_start (prototype => $prototype);
	    for my $i (0 .. $$prototype{'numargs'} - 1 ) {
		$self->function_arg (prototype => $prototype,
				     argtype   => $$prototype{'argtypes'}[$i],
				     argname   => $$prototype{'___argnames'}[$i],
				     argreg    => $$prototype{'regs'}[$i],
				     argnum    => $i );
	    }
	    $self->function_end (prototype => $prototype);
	    
	    print "\n";
	}
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#ifdef __cplusplus\n";
	print "}\n";
	print "#endif /* __cplusplus */\n";

	if ($self->{PROTO}) {
	    print "\n";
	    print "#endif /* _$$sfd{'BASENAME'}_H */\n";
	}
    }


    # Helper functions
    
    sub function_proto {
	my $self     = shift;
	my %params   = @_;
	my $prototype = $params{'prototype'};
	my $sfd      = $self->{SFD};
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($self->{PROTO}) {
	    print ";\n";
	}
	elsif (!$self->{LIBPROTO}) {
	    print "\n";
	    print "{\n";
	    print "  return $libprefix$prototype->{funcname}(";

	    if ($libarg eq 'first' && !$prototype->{nb}) {
		print "_base";
		print $prototype->{numargs} > 0 ? ", " : "";
	    }
	}
    }

    sub function_arg {
	my $self      = shift;

	if (!$self->{PROTO} && !$self->{LIBPROTO}) {
	    my %params    = @_;
	    my $argname   = $params{'argname'};
	    my $argnum    = $params{'argnum'};

	    print $argnum > 0 ? ", " : "";
	    print $argname;
	}
    }

    sub function_end {
	my $self      = shift;
	
	if (!$self->{PROTO} && !$self->{LIBPROTO}) {
	    my %params    = @_;
	    my $prototype = $params{'prototype'};
	    my $sfd       = $self->{SFD};
	    
	    if ($libarg eq 'last' && !$prototype->{nb}) {
		print $prototype->{numargs} > 0 ? ", " : "";
		print "_base";
	    }
	    
	    print ");\n";
	    print "}\n";
	}
    }

}
