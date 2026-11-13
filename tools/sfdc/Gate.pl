
### Class Gate: Create a generic gate file ####################################

BEGIN {
    package Gate;

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
	    print "#ifndef _GATEPROTO_$$sfd{'BASENAME'}_H\n";
	    print "#define _GATEPROTO_$$sfd{'BASENAME'}_H\n";
	}
	elsif ($self->{LIBPROTO}) {
	    print "/* Automatically generated header! Do not edit! */\n";
	    print "\n";
	    print "#ifndef _LIBPROTO_$$sfd{'BASENAME'}_H\n";
	    print "#define _LIBPROTO_$$sfd{'BASENAME'}_H\n";
	}
	else {
	    print "/* Automatically generated gatestubs! Do not edit! */\n";
	}
	print "\n";

	foreach my $inc (@{$$sfd{'includes'}}) {
	    print "#include $inc\n";
	}

	foreach my $td (@{$$sfd{'typedefs'}}) {
	    print "typedef $td;\n";
	}

	print "\n";
	print "#define _sfdc_strarg(a) _sfdc_strarg2(a)\n";
	print "#define _sfdc_strarg2(a) #a\n";

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
	elsif ($prototype->{type} eq 'cfunction') {
	    $self->function_proto (prototype => $prototype);

	    if (!$self->{LIBPROTO}) {
		print_gateproto ($sfd, $prototype);
		print ";\n\n";
	    }

	    if (!$self->{PROTO} && !$self->{LIBPROTO}) {
		print "__asm(\".globl \" _sfdc_strarg(" .
		    "$gateprefix$prototype->{funcname}) );\n";
		print "__asm(\".type  \" _sfdc_strarg(" .
		    "$gateprefix$prototype->{funcname}) \"" .
		    ", \@function\");\n";
		print "__asm(_sfdc_strarg(".
		    "$gateprefix$prototype->{funcname}) \":\");\n";
		print "#if defined(__mc68000__) || defined(__i386__)\n";
		print "__asm(\"jmp $libprefix$prototype->{funcname}\");\n";
		print "#else\n";
		print "# error \"Unknown CPU\"\n";
		print "#endif\n";
		print "\n";
	    }
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
	    print "#endif /* _GATEPROTO_$$sfd{'BASENAME'}_H */\n";
	}
	elsif ($self->{LIBPROTO}) {
	    print "\n";
	    print "#endif /* _LIBPROTO_$$sfd{'BASENAME'}_H */\n";
	}
    }


    # Helper functions
    
    sub function_proto {
	my $self     = shift;
	my %params   = @_;
	my $prototype = $params{'prototype'};
	my $sfd      = $self->{SFD};

	if (!$self->{PROTO}) {
	    if ($prototype->{type} eq 'varargs') {
		print_libproto($sfd, $prototype->{real_prototype});
	    }
	    else {
		print_libproto($sfd, $prototype);
	    }
	    print ";\n\n";
	}
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if (!$self->{LIBPROTO}) {
	    print_gateproto ($sfd, $prototype);
	}
	
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


    sub print_gateproto {
	my $sfd       = shift;
	my $prototype = shift;
	
	print "$prototype->{return}\n";
	print "$gateprefix$prototype->{funcname}(";

	if ($libarg eq 'first' && !$prototype->{nb}) {
	    print "$sfd->{basetype} _base";
	    print $prototype->{numargs} > 0 ? ", " : "";
	}
	
	print join (', ', @{$prototype->{___args}});

	if ($libarg eq 'last' && !$prototype->{nb}) {
	    print $prototype->{numargs} > 0 ? ", " : "";
	    print "$sfd->{basetype} _base";
	}

	if ($libarg eq 'none' && $prototype->{numargs} == 0) {
	    print "void";
	}

	print ")";
    }

    sub print_libproto {
	my $sfd       = shift;
	my $prototype = shift;
	
	print "$prototype->{return}\n";
	print "$libprefix$prototype->{funcname}(";

	if ($libarg eq 'first' && !$prototype->{nb}) {
	    print "$sfd->{basetype} _base";
	    print $prototype->{numargs} > 0 ? ", " : "";
	}

	print join (', ', @{$prototype->{___args}});

	if ($libarg eq 'last' && !$prototype->{nb}) {
	    print $prototype->{numargs} > 0 ? ", " : "";
	    print "$sfd->{basetype} _base";
	}

	if ($libarg eq 'none' && $prototype->{numargs} == 0) {
	    print "void";
	}
	
	print ")";
    }
}
