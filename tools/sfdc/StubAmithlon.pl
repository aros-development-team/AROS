
### Class StubAmithlon: Create a Amithlon stub file ###########################

BEGIN {
    package StubAmithlon;
    use vars qw(@ISA);
    @ISA = qw( Stub );

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
	print "ULONG _CallLib68k(struct _Regs*,LONG) " .
	    "__attribute__((__regparm__(3)));\n";
	print "\n";
	print "#endif /* __INLINE_MACROS_H */\n";
	print "\n";
    }
    
    sub function_start {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	if ($prototype->{type} eq 'function') {
	    print "\n";
	    print "{\n";

	    if (!$prototype->{nb}) {
		print "  BASE_EXT_DECL\n";
	    }
	    if (!$prototype->{nr}) {
		print "  $prototype->{return} _res;\n";
	    }

	    print "  struct _Regs _regs;\n";
	}
	else {
	    $self->SUPER::function_start (@_);
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

	if ($$prototype{'type'} eq 'function') {
	    printf "  __asm(\"movl %%1,%%0\":\"=m\"(_regs.%s)" .
		":\"ri\"((ULONG)%s));\n", $argreg, $argname;
	}
	else {
	    $self->SUPER::function_arg (@_);
	}
    }
    
    sub function_end {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	
	if ($$prototype{'type'} eq 'function') {
	    if (!$prototype->{nb}) {
		print "  __asm(\"movl %1,%0\":\"=m\"(_regs.a6)" .
		    ":\"ri\"((ULONG)(BASE_NAME)));\n";
	    }

	    print "  ";
	    
	    if (!$prototype->{nr}) {
		print "_res = ($prototype->{return}) ";
	    }

	    print "_CallLib68k(&_regs,-$prototype->{bias});\n";
	    
	    if (!$prototype->{nr}) {
		print "  return _res;\n";
	    }

	    print "}\n";
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }
}
