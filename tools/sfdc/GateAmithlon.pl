
### Class GateAmithlon: Create an Amithlon gatestub file ######################

BEGIN {
    package GateAmithlon;
    use vars qw(@ISA);
    @ISA = qw( Gate );

    sub new {
	my $proto  = shift;
	my $class  = ref($proto) || $proto;
	my $self   = $class->SUPER::new( @_ );
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	
	$self->SUPER::header (@_);

	print "#ifndef __INLINE_MACROS_H\n";
	print "#define __INLINE_MACROS_H\n";
	print "\n";
	print "#ifndef __INLINE_MACROS_H_REGS\n";
	print "#define __INLINE_MACROS_H_REGS\n";
	print "\n";
	print "#include <exec/types.h>\n";
	print "\n";
	print "struct _Regs {\n";
	print "	ULONG d0;\n";
	print "	ULONG d1;\n";
	print "	ULONG d2;\n";
	print "	ULONG d3;\n";
	print "	ULONG d4;\n";
	print "	ULONG d5;\n";
	print "	ULONG d6;\n";
	print "	ULONG d7;\n";
	print "	ULONG a0;\n";
	print "	ULONG a1;\n";
	print "	ULONG a2;\n";
	print "	ULONG a3;\n";
	print "	ULONG a4;\n";
	print "	ULONG a5;\n";
	print "	ULONG a6;\n";
	print "	ULONG a7;\n";
	print "};\n";
	print "\n";
	print "#endif /* __INLINE_MACROS_H_REGS */\n";
	print "\n";
	print "#endif /* __INLINE_MACROS_H */\n";
	print "\n";
    }

    sub function_proto {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	$self->SUPER::function_proto (@_);

	print_gateproto($sfd, $prototype);
	print ";\n\n";
    }
    
    sub function_start {
	my $self      = shift;

	if (!$self->{PROTO}) {
	    my %params    = @_;
	    my $prototype = $params{'prototype'};
	    my $sfd       = $self->{SFD};

	    print "$prototype->{return}\n";
	    print "$gateprefix$prototype->{funcname}(struct _Regs* _regs)\n";
	    print "{\n";
	}
    }

    sub function_arg {
	my $self      = shift;

	if (!$self->{PROTO}) {
	    my %params    = @_;
	    my $prototype = $params{'prototype'};
	    my $argtype   = $params{'argtype'};
	    my $argname   = $params{'argname'};
	    my $argreg    = $params{'argreg'};
	    my $argnum    = $params{'argnum'};
	    my $sfd       = $self->{SFD};

	    print "  $prototype->{___args}[$argnum] = ($argtype) ({long r;" .
		"__asm(\"movl %1,%0\":\"=r\"(r):\"m\"(_regs->$argreg));" .
		"r;});\n";
	}
    }
    
    sub function_end {
	my $self      = shift;

	if (!$self->{PROTO}) {
	    my %params    = @_;
	    my $prototype = $params{'prototype'};
	    my $sfd       = $self->{SFD};

	    if ($libarg ne 'none' && !$prototype->{nb}) {
		print "  $sfd->{basetype} _base = ($sfd->{basetype}) " .
		    "({long r;" .
		    "__asm(\"movl %1,%0\":\"=r\"(r):\"m\"(_regs->a6));" .
		    "r;});\n";
	    }

	    print "  return $libprefix$prototype->{funcname}(";

	    if ($libarg eq 'first' && !$prototype->{nb}) {
		print "_base";
		print $prototype->{numargs} > 0 ? ", " : "";
	    }

	    print join (', ', @{$prototype->{___argnames}});
	
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
	print "$gateprefix$prototype->{funcname}" .
	    "(struct _Regs* _regs) __attribute__((regparm(3)))";
    }
}
