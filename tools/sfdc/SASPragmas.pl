
### Class SASPragmas: Create a SAS/C pragmas file #############################

BEGIN {
    package SASPragmas;

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

	my $id = $$sfd{'id'};
	my $v  = $id;
	my $d  = $id;

	$v =~ s/^\$[I]d: .*? ([0-9.]+).*/$1/;
	$d =~ s,^\$[I]d: .*? [0-9.]+ (\d{4})/(\d{2})/(\d{2}).*,($3.$2.$1),;

	print "/* Automatically generated header! Do not edit! */\n";
	print "#ifndef PRAGMAS_$sfd->{BASENAME}_PRAGMAS_H\n";
	print "#define PRAGMAS_$sfd->{BASENAME}_PRAGMAS_H\n";
	print "\n";
	print "/*\n";
	print "**	\$VER: $$sfd{'basename'}_pragmas.h $v $d\n";
	print "**\n";
	print "**	Direct ROM interface (pragma) definitions.\n";
	print "**\n";
	print "**	$$sfd{'copyright'}\n";
	print "**	    All Rights Reserved\n";
	print "*/\n";
	print "\n";

	print "#if defined(LATTICE) || defined(__SASC) || defined(_DCC)\n";
	print "#ifndef __CLIB_PRAGMA_LIBCALL\n";
	print "#define __CLIB_PRAGMA_LIBCALL\n";
	print "#endif /* __CLIB_PRAGMA_LIBCALL */\n";
	print "#else /* __MAXON__, __STORM__ or AZTEC_C */\n";
	print "#ifndef __CLIB_PRAGMA_AMICALL\n";
	print "#define __CLIB_PRAGMA_AMICALL\n";
	print "#endif /* __CLIB_PRAGMA_AMICALL */\n";
	print "#endif /* */\n";
	print "\n";
	print "#if defined(__SASC_60) || defined(__STORM__)\n";
	print "#ifndef __CLIB_PRAGMA_TAGCALL\n";
	print "#define __CLIB_PRAGMA_TAGCALL\n";
	print "#endif /* __CLIB_PRAGMA_TAGCALL */\n";
	print "#endif /* __MAXON__, __STORM__ or AZTEC_C */\n";
	print "\n";
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	# Don't process private functions
	if ($prototype->{private}) {
	    return;
	}

	my $regs = '';
	
	foreach my $reg (@{$prototype->{regs}}) {
	    my $num;
	    
	    if ($reg =~ /^d[0-7]$/) {
		($num) = $reg =~ /^d(.)/;
	    }
	    elsif ($reg =~ /^a[0-9]$/) {
		($num) = $reg =~ /^a(.)/;
		$num += 8;
	    }
	    else {
		die;
	    }

	    $regs = sprintf "%x$regs", $num;
	}

	$regs .= '0'; #Result in d0
	$regs .= $prototype->{numregs};
	
	if ($prototype->{type} eq 'function') {
	    # Always use libcall, since access to 4 is very expensive

	    print "#ifdef __CLIB_PRAGMA_LIBCALL\n";
	    print " #pragma libcall $sfd->{base} $prototype->{funcname} ";
	    printf "%x $regs\n", $prototype->{bias};
	    print "#endif /* __CLIB_PRAGMA_LIBCALL */\n";
	    print "#ifdef __CLIB_PRAGMA_AMICALL\n";
	    printf " #pragma amicall($sfd->{base}, 0x%x, $prototype->{funcname}(",
		$prototype->{bias};
	    print join (',', @{$prototype->{regs}}) . "))\n";
	    print "#endif /* __CLIB_PRAGMA_AMICALL */\n";
	}
	elsif ($prototype->{type} eq 'varargs') {
	    print "#ifdef __CLIB_PRAGMA_TAGCALL\n";
	    print " #ifdef __CLIB_PRAGMA_LIBCALL\n";
	    print "  #pragma tagcall $sfd->{base} $prototype->{funcname} ";
	    printf "%x $regs\n", $prototype->{bias};
	    print " #endif /* __CLIB_PRAGMA_LIBCALL */\n";
	    print " #ifdef __CLIB_PRAGMA_AMICALL\n";
	    printf "  #pragma tagcall($sfd->{base}, 0x%x, $prototype->{funcname}(",
		$prototype->{bias};
	    print join (',', @{$prototype->{regs}}) . "))\n";
	    print " #endif /* __CLIB_PRAGMA_AMICALL */\n";
	    print "#endif /* __CLIB_PRAGMA_TAGCALL */\n";
	}
	elsif ($prototype->{type} eq 'cfunction') {
	    # Do nothing
	}
	else {
	    print STDERR "$prototype->{funcname}: Unsupported function " .
		"type.\n";
	    die;
	}
    }
    
    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#endif /* PRAGMAS_$sfd->{BASENAME}_PRAGMAS_H */\n";
    }
}
