BEGIN {
    # Read rules
    file=TOP "/config/make.tmpl";

    while ((getline < file) > 0)
    {
	if (substr($1,1,1) == "%")
	{
	    if ($1 == "%define")
	    {
		cmd = $2;
#print "Adding "cmd
		commands[cmd] = 1;

		for (t=3; t<=NF; t++)
		{
		    arg[cmd,t-2] = $t;
		}

		narg[cmd] = NF-2;

#printf ("Args=%d\n", NF-2);
#for (t=1; t<=NF-2; t++)
#    printf ("Arg[%d] = %s\n", t, arg[cmd,t]);

		t = 1;

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
}
/%[a-zA-Z_]+/ {
    if (match($0,/%[a-zA-Z_]+/))
    {
	name = substr($0,RSTART+1,RLENGTH-1);
#print "name "name

	if (name in commands)
	{
	    # Extract arguments
	    argstr=substr($0,RSTART+RLENGTH);
#print "Argstr="argstr
	    sub("^[ \t]+","",argstr);
#print "Argstr2="argstr
	    nargs=split(argstr,args);
#print "nargs="nargs

	    # Fill in defaults
	    for (t=nargs; t<=narg[name]; t++)
	    {
		args[t] = arg[name,t];
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
		str=body[name,t];
#printf ("Body[%d] = %s\n", t, body[name,t]);

		for (a=1; a<=10; a++)
		{
		    gsub("%"a,args[a],str);
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
