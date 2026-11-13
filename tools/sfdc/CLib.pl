
### Class CLib: Create a clib file ############################################

BEGIN {
    package CLib;

    sub new {
	my $proto  = shift;
	my %params = @_;
	my $class  = ref($proto) || $proto;
	my $self   = {};
	$self->{SFD}     = $params{'sfd'};
	$self->{VERSION} = 1;
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
	print "\n";
	print "#ifndef CLIB_$$sfd{'BASENAME'}_PROTOS_H\n";
	print "#define CLIB_$$sfd{'BASENAME'}_PROTOS_H\n";
	print "\n";
	print "/*\n";
	print "**	\$VER: $$sfd{'basename'}_protos.h $v $d\n";
	print "**\n";
	print "**	C prototypes. For use with 32 bit integers only.\n";
	print "**\n";
	print "**	$$sfd{'copyright'}\n";
	print "**	    All Rights Reserved\n";
	print "*/\n";
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

	$self->{VERSION} = 1;
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
	
	if ($self->{VERSION} != $$prototype{'version'}) {
	    $self->{VERSION} = $$prototype{'version'};

	    print "\n";
	    print "/*--- functions in V$self->{VERSION} or higher ---*/\n";
	}
	
	if ($$prototype{'comment'} ne '') {
	    my $comment = $$prototype{'comment'};

	    $comment =~ s,^(\s?)(.*)$,/*$1$2$1*/,mg;
		
	    print "\n";
	    print "$comment\n";
	}
	
	my $args = join (', ',@{$$prototype{'args'}});

	if ($args eq '') {
	    $args = "void";
	}
	
	print "$$prototype{'return'} $$prototype{'funcname'}($args)";

	if ($$classes{'target'} eq 'morphos' &&
	    $$prototype{'type'} eq 'varargs' &&
	    $$prototype{'subtype'} ne 'tagcall') {
	    print " __attribute__((varargs68k))";
	}
	
	print ";\n";
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "\n";
	print "#ifdef __cplusplus\n";
	print "}\n";
	print "#endif /* __cplusplus */\n";
	print "\n";
	print "#endif /* CLIB_$$sfd{'BASENAME'}_PROTOS_H */\n";
    }
}
