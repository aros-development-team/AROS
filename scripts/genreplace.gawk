#   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Create mf.inc files from the %add_objects,%exclude_files stuff
#
BEGIN {
	lastdir = "";
}
{
	match($2,/(.*\/)+/);
	funct = substr($2,RLENGTH + 1);
	path = substr($2,1,RLENGTH);

	if((path != lastdir) && (lastdir != ""))
	{
		file = GENDIR "/" lastdir "mf.inc";
		printf "SUPPRESS_FILES = %s\n", suppress > file;
		printf "ADDITIONAL_OBJS = %s\n", add_objs >> file;
		close(file);
#		printf "Adding information for %s\n", lastdir;
	
		suppress = "";
		add_objs = "";	
	}

	lastdir = path;

	if( $1 == "add" )
		add_objs =  add_objs " " funct;
	if( $1 == "not" )
		suppress = suppress " " funct;
}

END {
		file = GENDIR "/" lastdir "/mf.inc";
                printf "SUPPRESS_FILES = %s\n", suppress > file;
                printf "ADDITIONAL_OBJS = %s\n", add_objs >> file;
                close(file);
}
