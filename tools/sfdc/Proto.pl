
### Class Proto: Create a proto file ##########################################

BEGIN {
    package Proto;

    sub new {
	my $proto    = shift;
	my %params   = @_;
	my $class    = ref($proto) || $proto;
	my $self     = {};
	$self->{SFD} = $params{'sfd'};
	bless ($self, $class);
	return $self;
    }

    sub header {
	my $self = shift;
	my $sfd  = $self->{SFD};

	my $base      = $$sfd{'base'};
	my $basename  = $$sfd{'basename'};
	my $BASENAME  = $$sfd{'BASENAME'};
	my $BaseName  = $$sfd{'BaseName'};
	my $basetype  = $$sfd{'basetype'};

	print "/* Automatically generated header! Do not edit! */\n";
	print "\n";
	print "#ifndef PROTO_${BASENAME}_H\n";
	print "#define PROTO_${BASENAME}_H\n";
	print "\n";
	print "#include <clib/${basename}_protos.h>\n";
	print "\n";
	print "#ifndef _NO_INLINE\n";
	print "# if defined(__GNUC__)\n";
	print "#  ifdef __AROS__\n";
	print "#   include <defines/${basename}.h>\n";
	print "#  else\n";
	print "#   include <inline/${basename}.h>\n";
	print "#  endif\n";
	print "# else\n";
	print "#  include <pragmas/${basename}_pragmas.h>\n";
	print "# endif\n";
	print "#endif /* _NO_INLINE */\n";
	print "\n";

	if ($base ne '') {
	    print "#ifdef __amigaos4__\n";
	    print "# include <interfaces/${basename}.h>\n";
	    print "# ifndef __NOGLOBALIFACE__\n";
	    print "   extern struct ${BaseName}IFace *I${BaseName};\n";
	    print "# endif /* __NOGLOBALIFACE__*/\n";  
	    print "#else /* !__amigaos4__ */\n";
	    print "# ifndef __NOLIBBASE__\n";
	    print "   extern ${basetype}\n";
	    print "#  ifdef __CONSTLIBBASEDECL__\n";
	    print "    __CONSTLIBBASEDECL__\n";
	    print "#  endif /* __CONSTLIBBASEDECL__ */\n";
	    print "   ${base};\n";
	    print "# endif /* !__NOLIBBASE__ */\n";
	    print "#endif /* !__amigaos4__ */\n";
	    print "\n";
	}
    }

    sub function {
	# Nothing to do here ...
    }

    sub footer {
	my $self = shift;
	my $sfd  = $self->{SFD};

	print "#endif /* !PROTO_$$sfd{'BASENAME'}_H */\n";
    }
}
