
### Class StubMOS: Create a MorphOS stub file #################################

BEGIN {
    package StubMOS;
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

	print "\n";
	print "#include <emul/emulregs.h>\n";
	print "#include <stdarg.h>\n";
	print "\n";
    }

    sub function_proto {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};

	if ($prototype->{type} eq 'varargs') {

	    if ($prototype->{subtype} ne 'tagcall') {
		# We have to add the attribute to ourself first
	    
		$self->special_function_proto (@_);
		print " __attribute__((varargs68k));\n";
		print "\n";
		$self->special_function_proto (@_);
	    }
	}
	else {
	    $self->SUPER::function_proto (@_);
	}
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
	}
	elsif ($prototype->{type} eq 'varargs') {
	    if ($prototype->{subtype} ne 'tagcall') {
		my $na;

		if ($prototype->{subtype} eq 'printfcall') {
		    $na = $prototype->{numargs} - 2;
		}
		else {
		    # methodcall: first vararg is removed
		    $na = $prototype->{numargs} - 3;
		}
		
		print "\n";
		print "{\n";
		print "  va_list _va;\n";
		print "  va_start (_va, $prototype->{___argnames}[$na]);\n";
		print "  return $$prototype{'real_funcname'}(BASE_PAR_NAME ";
	    }
	    else {
		# Shamelessly stolen from fd2inline ...
		
		# number of regs that contain varargs
		my $n = 9 - $prototype->{numregs};

		# add 4 bytes if that's an odd number, to avoid splitting a tag
		my $d = $n & 1 ? 4 : 0;

		# offset of the start of the taglist
		my $taglist = 8;

		# size of the stack frame
		my $local = ($taglist + $n * 4 + $d + 8 + 15) & ~15;

		#  Stack frame:
		#
		#   0 -  3: next frame ptr
		#   4 -  7: save lr
		#   8 -  8+n*4+d+8-1: tag list start
		#   ? - local-1: padding

		print  "__asm(\"\n";
		print  "	.align	2\n";
		print  "	.globl	$prototype->{funcname}\n";
		print  "	.type	$prototype->{funcname},\@function\n";
		print  "$prototype->{funcname}:\n";
		print  "	stwu	1,-$local(1)\n";
		print  "	mflr	0\n";
		printf "	stw	0,%d(1)\n", $local + 4;

		# If n is odd, one tag is split between regs and stack.
		# Copy its ti_Data together with the ti_Tag.
	    
		if ($d != 0) {
		    # read ti_Data
		    printf "	lwz	0,%d(1)\n", $local + 8;
		}

		# Save the registers
	    
		for my $count ($prototype->{numregs} .. 8) {
		    printf "	stw	%d,%d(1)\n",
		    $count + 2,
		    ($count - $prototype->{numregs}) * 4 + $taglist;
		}

		if ($d != 0) {
		    # write ti_Data
		    printf "	stw	0,%d(1)\n", $taglist + $n * 4;
		}

		# Add TAG_MORE

		print  "	li	11,2\n";
		printf "	addi	0,1,%d\n", $local + 8 + $d;
		printf "	stw	11,%d(1)\n", $taglist + $n * 4 + $d;
		printf "	stw	0,%d(1)\n", $taglist + $n * 4 + $d + 4;

		# vararg_reg = &saved regs
	    
		printf "	addi	%d,1,%d\n",
		$prototype->{numregs} + 2, $taglist;
		print "	bl	$prototype->{real_funcname}\n";
	    }
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
	my $sfd       = $self->{SFD};

	if ($$prototype{'type'} eq 'function') {
	    print "  REG_" . (uc $argreg) . " = (ULONG) $argname;\n";
	}
	elsif ($prototype->{type} eq 'varargs') {
	    if ($prototype->{subtype} eq 'tagcall') {
#	    if ($argnum < $prototype->{numargs} - 2) {
#		my $regoffset;
#
#		if ($argreg =~ /^d[0-9]$/) {
#		    ( $regoffset = $argreg ) =~ s/^d//;
#		}
#		elsif ($argreg =~ /^a[0-9]$/) {
#		    ( $regoffset = $argreg ) =~ s/^a//;
#		    $regoffset += 8;
#		}
#		else {
#		    die;
#		}
#
#		$regoffset *= 4;
#
#		# Save the non-varargs registers in the EmulHandle struct
#
#		printf "	stw	%d,%d(2)\n", $argnum + 3, $regoffset;
#	    }
	    }
	    elsif ($prototype->{subtype} eq 'methodcall' &&
		   $argnum == $prototype->{numargs} - 2) {
		# Nuke it!
	    }
	    elsif ($argnum == $prototype->{numargs} - 1) {
		my $vt  = $$prototype{'argtypes'}[$$prototype{'numargs'} - 1];
		print ", ($vt) _va->overflow_arg_area";
	    }
	    else {
		$self->SUPER::function_arg (@_);
	    }
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
		print "  REG_A6 = (ULONG) BASE_NAME;\n";
	    }

	    print "  ";
	    
	    if (!$prototype->{nr}) {
		print "return ($prototype->{return}) ";
	    }

	    print "(*MyEmulHandle->EmulCallDirectOS)(-$prototype->{bias});\n";
	    print "}\n";
	}
	elsif ($prototype->{type} eq 'varargs') {
	    if ($prototype->{subtype} eq 'tagcall') {
		# number of regs that contain varargs
		my $n = 9 - $prototype->{numregs};

		# add 4 bytes if that's an odd number, to avoid splitting a tag
		my $d = $n & 1 ? 4 : 0;

		# offset of the start of the taglist
		my $taglist = 8;

		# size of the stack frame
		my $local = ($taglist + $n * 4 + $d + 8 + 15) & ~15;

		# clear stack frame & return
		printf "	lwz	0,%d(1)\n", $local + 4;
		print  "	mtlr	0\n";
		printf "	addi	1,1,%d\n", $local;
		print  "	blr\n";
		print  ".L$prototype->{funcname}e1:\n";
		print  "	.size	$prototype->{funcname}," .
		    ".L$prototype->{funcname}e1-$prototype->{funcname}\n";

		print "\");\n";
	    }
	    else {
		print ");\n";
		print "}\n";
	    }
	}
	else {
	    $self->SUPER::function_end (@_);
	}
    }


    sub special_function_proto {
	my $self     = shift;
	my %params   = @_;
	my $prototype    = $params{'prototype'};
	my $decl_regular = $params{'decl_regular'};
	my $sfd      = $self->{SFD};

	if ($prototype->{type} eq 'varargs' && $decl_regular) {
	    my $rproto = $prototype->{real_prototype};

	    print "$$rproto{'return'} $$rproto{'funcname'}(";
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
	
	print "$$prototype{'return'}\n";
	print "$$prototype{'funcname'}(";
	if (!$prototype->{nb}) {
	    if ($$prototype{'numargs'} == 0) {
		print "BASE_PAR_DECL0";
	    }
	    else {
		print "BASE_PAR_DECL ";
	    }
	}

	my @newargs;

	for my $i (0 .. $#{@{$prototype->{___args}}}) {
	    if ($prototype->{subtype} ne 'methodcall' ||
		$i != $prototype->{numargs} - 2 ) {
		push @newargs, $prototype->{___args}[$i];
	    }
	}

	print join (', ', @newargs);
	print ")";
	
    }
}
