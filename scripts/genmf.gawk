BEGIN {
    # Read rules
    file=TOP "/config/make.tmpl";
    has_common = 0;

    while ((getline < file) > 0)
    {
	if (substr($1,1,1) == "%")
	{
	    if ($1 == "%define")
	    {
		# The name of the command is the second argument
		cmd = $2;
#print "Adding "cmd
		commands[cmd] = 1;

		argc = 1;

		# Process the arguments of the macro
		for (t=3; t<=NF; t++)
		{
		    # Get the name of the argument and it's default
		    n = split($t,opt,"=");
		    arg[cmd,argc] = opt[1];
		    cmdarg[cmd,opt[1]] = argc;

		    opts = opt[2];

		    # If the default begins with "...
		    if (substr (opts,1,1) == "\"")
		    {
			# Strip first "
			opts = substr(opts,2);

			while (t<=NF)
			{
			    # Search for the terminating "
			    if (substr(opts,length(opts)) == "\"")
			    {
				# Strip the trailing "
				opts=substr(opts,1,length(opts)-1);
				break;
			    }

			    t++;

			    opts = opts " " $t;
			}
		    }
#print "n="n " arg="arg[cmd,argc] " default="opts

		    # Found a default ?
		    if (n == 2)
		    {
			# Set start values
			default[cmd,argc]=opts;
			flags[cmd,argc]="";

			# Strip flags off defaults
			while (1)
			{
			    flag=toupper(substr(opts,length(opts)-1));
			    # Check if this is a flag
			    if (flag=="/M")
			    {
				# Remove flag from default string
				opts = substr(opts,1,length(opts)-2);
				default[cmd,argc]=opts;
				flags[cmd,argc]=flag flags[cmd,argc];
			    }
			    else
				break;
			}
		    }
		    else
		    {
			# No default and no flags
			default[cmd,argc]="";
			flags[cmd,argc]="";
		    }

		    # Strip the /'s from the flags
		    gsub("/","",flags[cmd,argc]);

		    # Count one argument
		    argc ++;
		}

		# Store the number of arguments
		narg[cmd] = argc-1;

#printf ("Args=%d\n", narg[cmd]);
#for (t=1; t<=narg[cmd]; t++)
#{
#    printf ("Arg[%d] = %s default=%s flags=%s\n", t, arg[cmd,t],
#	 default[cmd,t], flags[cmd,t]);
#}
		t = 1;

		# Read the body
		while ((getline < file) > 0)
		{
		    if ($0 == "%end")
			break;

		    body[cmd,t++] = $0;
		}

		nbody[cmd] = t-1;
	    }
	    else
	    {
		printf ("Unknown command: %s (ignored)\n", $1);
	    }
	}
    }

#    exit (0);
}
/^#[ \t]+\$Id/ { next }
/%[a-zA-Z_]+/ {
    if (match($0,/%[a-zA-Z_]+/))
    {
	# Get name of macro
	name = substr($0,RSTART+1,RLENGTH-1);
#print "name "name

	if (name in commands)
	{
	    if (name == "common")
		has_common = 1;

	    # Extract arguments
	    argstr=substr($0,RSTART+RLENGTH);
	    # Remove leading spaces
	    sub("^[ \t]+","",argstr);
#print "Argstr="argstr
	    # Split args by spaces
	    nargs=split(argstr,userargs);
#print "nargs="nargs

	    # Fill in defaults
	    for (t=1; t<=narg[name]; t++)
	    {
		args[t] = default[name,t];
	    }

	    argc = 1;

	    # Overwrite defaults by user supplied values.
	    for (t=1; t<=nargs; t++)
	    {
#print t": "userargs[t];
		# Check if the user has specified the name of the argument.
		n = split(userargs[t],opt,"=");

		if (n==1)
		{
		    # Store that argument by position
		    args[argc] = userargs[t];

		    # Check flags
		    if (index(flags[name,argc],"M"))
		    {
			# Collect the rest in this argument.
			# I don't check for "" here. This allows to keep
			# arguments together because they are passed on.
			for (t++; t<=nargs; t++)
			{
			    args[argc] = args[argc] " " userargs[t];
			}
		    }
		    else if (substr(args[argc],1,1) == "\"")
		    {
			# Search second " and strip them.

			while (1)
			{
			    if (substr(args[argc],length(args[argc]))=="\"")
			    {
				args[argc] = substr(args[argc],2,length(args[argc])-2);
				break;
			    }

			    args[argc] = args[argc] " " userargs[++t];
			}
		    }

		    argc ++;
		}
		else
		{
		    # Search the argument by it's name
		    if (cmdarg[name,opt[1]] != 0)
		    {
			x = cmdarg[name,opt[1]];

			args[x] = opt[2];

			if (index(flags[name,x],"M"))
			{
			    # Same as abouve
			    for (t++; t<=nargs; t++)
			    {
				args[x] = args[x] " " userargs[t];
			    }
			}
			else if (substr(args[x],1,1) == "\"")
			{
			    # Search second " and strip them.

			    while (1)
			    {
				if (substr(args[x],length(args[x]))=="\"")
				{
				    args[x] = substr(args[x],2,length(args[x])-2);
				    break;
				}

				args[x] = args[x] " " userargs[++t];
			    }
			}
		    }
		    else
		    {
			printf ("Error: Unknown argument %s!\n", opt[1]);
		    }
		}
	    }

	    if (narg[name] > nargs)
		nargs = narg[name];

#printf ("Args=%d\n", nargs);
#for (t=1; t<=nargs; t++)
#    printf ("Arg[%d] = %s\n", t, args[t]);
#printf ("nbody=%d\n", nbody[name]);

	    # Emit the body
	    for (t=1; t<=nbody[name]; t++)
	    {
		# Get the next line
		str=body[name,t];
#printf ("Body[%d] = %s\n", t, body[name,t]);

		# Replace all arguments in that line
		for (a=1; a<=nargs; a++)
		{
		    gsub("%[(]"arg[name,a]"[)]",args[a],str);
		}

		print str
	    }

	    next;
	}
    }
    else
	print;
}
 { print }
END {
    if (!has_common)
    {
	name="common";
	for (t=1; t<=nbody[name]; t++)
	    print body[name,t];
    }
}
