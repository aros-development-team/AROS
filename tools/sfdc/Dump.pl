
### Class Dump: Dump SFD info #################################################

BEGIN {
    package Dump;

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

	print "SFD information\n";
	print "¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\n";
	print "Copyright:		$$sfd{'copyright'}\n";
	print "RCS ID:			$$sfd{'id'}\n";
	print "Module name:		$$sfd{'libname'}\n";
	print "Module base:		$$sfd{'base'}\n";
	print "Module base type:	$$sfd{'basetype'}\n";
	print "Module base names:	$$sfd{'basename'}, $$sfd{'BASENAME'}, ";
	print "$$sfd{'Basename'}\n";
	print "\n";
	print "Include files:		";
	print join ("\n			", @{$$sfd{'includes'}});
	print "\n";
	print "Type definitions:		";
	if ($#{@{$sfd->{typedefs}}} != -1) {
	    print join ("\n			", @{$$sfd{'typedefs'}});
	}
	print "\n";
	print "\n";
    }

    sub function {
	my $self      = shift;
	my %params    = @_;
	my $prototype = $params{'prototype'};
	my $sfd       = $self->{SFD};

	print "* Line $$prototype{'line'}: $$prototype{'funcname'}()\n";
	print "	Type:			" . ucfirst $$prototype{'type'} . "\n";
	print "	Subtype:		$prototype->{subtype}\n";
	if ($prototype->{real_funcname} ne '') {
	    print "	Real function name:\t$$prototype{'real_funcname'}\n";
	}
	print "	Visibility:		";
	print $$prototype{'private'} == 0 ? "Public\n" : "Private\n";
	print "	Library offset/bias:	-$$prototype{'bias'}\n";
	print "	Available since:	V$$prototype{'version'}\n";
	print "	Comment:		$$prototype{'comment'}\n";
	print "\n";
	print "	No return:		";
	print $prototype->{nr} ? "Yes\n" : "No\n";
	print "	No base:		";
	print $prototype->{nb} ? "Yes\n" : "No\n";
	print "\n";
	print "	Return value:		$$prototype{'return'}\n";
	print "	Arguments:		";
	print join (",\n\t\t\t\t", @{$$prototype{'args'}});
	print "\n";
	print "	Argument names:		";
	print join (", ", @{$$prototype{'argnames'}});
	print "\n";
	print "	Local arguments:	";
	print join (",\n\t\t\t\t", @{$$prototype{'___args'}});
	print "\n";
	print "	Local argument names:	";
	print join (", ", @{$$prototype{'___argnames'}});
	print "\n";
	print "	Argument types:		";
	print join (",\n\t\t\t\t", @{$$prototype{'argtypes'}});
	print "\n";

	print "\n";

#		value   => $proto_line,

#    $$prototype{'return'}     = $return;
#    $$prototype{'funcname'}   = $name;
#    @{$$prototype{'args'}}     = ();
#    @{$$prototype{'regs'}} = split(/,/,lc $registers);  # Make regs lower case
#    @{$$prototype{'argnames'}} = ();                    # Initialize array
#    @{$$prototype{'argtypes'}} = ();                    # Initialize array
    }

    sub footer {
	print "\n";
    }
}
