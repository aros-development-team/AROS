
### Class Stub: Create a generic stub file ####################################

BEGIN {
    package Stub;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}     = $params{'sfd'};
	$self->{NEWFILE} = 0;
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	$self->{NEWFILE} = 1;

	print "/* Automatically generated stubs! Do not edit! */\n";
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

	if ($$sfd{'base'} ne '') { 
	    print "#ifndef BASE_EXT_DECL\n";
	    print "#define BASE_EXT_DECL\n";
	    print "#define BASE_EXT_DECL0 extern $$sfd{'basetype'} " .
		"$$sfd{'base'};\n";
	    print "#endif /* !BASE_EXT_DECL */\n";
	    print "#ifndef BASE_PAR_DECL\n";
	    print "#define BASE_PAR_NAME\n";
	    print "#define BASE_PAR_DECL\n";
	    print "#define BASE_PAR_DECL0 void\n";
	    print "#endif /* !BASE_PAR_DECL */\n";
	    print "#ifndef BASE_NAME\n";
	    print "#define BASE_NAME $$sfd{'base'}\n";
	    print "#endif /* !BASE_NAME */\n";
	    print "\n";
	    print "BASE_EXT_DECL0\n";
	    print "\n";
	}

    }

    sub function {
	my $self     = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	# Don't process private functions
	if ($prototype->{private}) {
	    return;
	}
	
	$self->function_proto (prototype => $prototype, decl_regular => $self->{NEWFILE} );
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

	$self->{NEWFILE} = 0;
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#undef BASE_EXT_DECL\n";
	print "#undef BASE_EXT_DECL0\n";
	print "#undef BASE_PAR_NAME\n";
	print "#undef BASE_PAR_DECL\n";
	print "#undef BASE_PAR_DECL0\n";
	print "#undef BASE_NAME\n";
	print "\n";
	print "#ifdef __cplusplus\n";
	print "}\n";
	print "#endif /* __cplusplus */\n";
    }


    # Helper functions
    
    sub function_proto {
	my $self     = shift;
	my %params   = @_;
	my $prototype    = $params{'prototype'};
	my $decl_regular = $params{'decl_regular'};
	my $sfd      = $self->{SFD};

	if ($prototype->{type} eq 'varargs' && $decl_regular) {
	    my $rproto = $prototype->{real_prototype};

	    print "__inline $$rproto{'return'} $$rproto{'funcname'}(";
	    if (!$prototype->{nb}) {
		if ($$rproto{'numargs'} == 0) {
		    print "BASE_PAR_DECL0";
		}
		else {
		    print "BASE_PAR_DECL ";
		}
	    }
	    print join (', ', @{$$rproto{'___args'}});

	    print ");\n";
	    print "\n";
	}

	if ($prototype->{type} eq 'cfunction' &&
	    $prototype->{argnames}[$#{@{$prototype->{argnames}}}] eq '...') {
	    print "#if 0\n";
	    print "/* Unsupported */\n";
	}

	# Declare structs in case some ==include directive is missing
	for my $argtype (@{$prototype->{argtypes}}) {
	    my $struct;

	    (undef, $struct) = ( $argtype =~ /\s*(const)?\s*struct\s*(\w+).*/) and
		printf "struct $struct;\n";
	}

	
	print "__inline $$prototype{'return'}\n";
	print "$$prototype{'funcname'}(";
	if (!$prototype->{nb}) {
	    if ($$prototype{'numargs'} == 0) {
		print "BASE_PAR_DECL0";
	    }
	    else {
		print "BASE_PAR_DECL ";
	    }
	}
	print join (', ', @{$$prototype{'___args'}});
	print ")";
	
    }

    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	print "\n";
	print "{\n";

	if ($$prototype{'type'} eq 'varargs') {
	    print "  return $$prototype{'real_funcname'}(BASE_PAR_NAME ";
	}
	elsif ($prototype->{type} eq 'cfunction') {
	    if (!$prototype->{nb}) {
		print "  BASE_EXT_DECL\n";
	    }

	    my $argtypes = join (', ',@{$$prototype{'argtypes'}});

	    if ($argtypes eq '') {
		if ($prototype->{nb}) {
		    $argtypes = "void";
		}
	    }
	    else {
		if (!$prototype->{nb}) {
		    $argtypes = "$sfd->{basetype}, $argtypes";
		}
	    }


	    # Skip jmp instruction (is m68k ILLEGAL in MOS)
	    my $offs = $$prototype{'bias'} - 2;
	    
	    print "  $$prototype{'return'} (*_func) ($argtypes) = \n";
	    print "    ($$prototype{'return'} (*) ($argtypes))\n";
	    print "    *((ULONG*) (((char*) BASE_NAME) - $offs));\n";
	    print "  return (*_func)(";

	    if (!$prototype->{nb}) {
		print "BASE_NAME";
		print ", " unless $prototype->{numargs} == 0;
	    }
	}
	else {
	    print STDERR "$prototype->{funcname}: Unhandled.\n";
	    die;
	}
    }

    sub function_arg {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $argtype   = $params{'argtype'};
	my $argname   = $params{'argname'};
	my $argreg    = $params{'argreg'};
	my $argnum    = $params{'argnum'};
	my $sfd       = $self->{SFD};

	my $argstr;
	
	if ($$prototype{'type'} eq 'varargs') {
	    if ($prototype->{subtype} eq 'printfcall') {
		if ($argnum < $$prototype{'numargs'} - 1) {
		    $argstr = $argname;
		}
		elsif ($argnum == $$prototype{'numargs'} - 1) {
		    my $vartype  =
			$$prototype{'argtypes'}[$$prototype{'numargs'} - 1];
		    my $argnm =
			$$prototype{'___argnames'}[$$prototype{'numargs'} - 2];
		    $argstr = "($vartype) (&$argnm + 1)";
		}
		else {
		    $argstr = '';
		}
	    }
	    else {
		# tagcall/methodcall
		if ($argnum < $$prototype{'numargs'} - 2) {
		    $argstr = $argname;
		}
		elsif ($argnum == $$prototype{'numargs'} - 2) {
		    my $vartype =
			$$prototype{'argtypes'}[$$prototype{'numargs'} - 1];
		    $argstr = "($vartype) &$argname";
		}
		else {
		    $argstr = '';
		}
	    }
	}
	elsif ($prototype->{type} eq 'cfunction') {
	    $argstr = $argname;
	}
	else {
	    print STDERR "$prototype->{funcname}: Unhandled.\n";
	    die;
	}

	if ($argstr ne '') {
	    print ($argnum != 0 ? ", $argstr" : $argstr);
	}
    }

    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};
	
	print ");\n";
	print "}\n";

	if ($prototype->{type} eq 'cfunction' &&
	    $prototype->{argnames}[$#{@{$prototype->{argnames}}}] eq '...') {
	    print "/* Unsupported */\n";
	    print "#endif\n";
	}
    }
}
