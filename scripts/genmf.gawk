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
		    n = split($t,opt,"=");
#print "n="n " opt[1]="opt[1] " opt[2]="opt[2]
		    arg[cmd,t-2] = opt[1];

		    if (n == 2)
		    {
			n = index(opt[2],"/");
			if (!n)
			{
			    default[cmd,t-2]=opt[2];
			    flags[cmd,t-2]="";
			}
			else
			{
			    default[cmd,t-2]=substr(opt[2],1,n-1);
			    flags[cmd,t-2]=toupper(substr(opt[2],n+1));
			}
		    }
		    else
		    {
			default[cmd,t-2]="";
			flags[cmd,t-2]="";
		    }

		    gsub("/","",flags[cmd,t-2]);
		}

		narg[cmd] = NF-2;

#printf ("Args=%d\n", narg[cmd]);
#for (t=1; t<=narg[cmd]; t++)
#{
#    printf ("Arg[%d] = %s default=%s flags=%s\n", t, arg[cmd,t],
#	 default[cmd,t], flags[cmd,t]);
#}
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

#    exit (0);
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
	    nargs=split(argstr,userargs);
#print "nargs="nargs

	    # Fill in defaults
	    for (t=1; t<=narg[name]; t++)
	    {
		args[t] = default[name,t];
	    }

	    argc = 1;

	    for (t=1; t<=nargs; t++)
	    {
		n = split(userargs[t],opt,"=");

		if (n==1)
		{
		    args[argc] = userargs[t];

		    if (index(flags[name,argc],"M"))
		    {
			for (t++; t<=nargs; t++)
			{
			    args[argc] = args[argc] " " userargs[t];
			}
		    }

		    argc ++;
		}
		else
		{
		    for (x=1; x<narg[name]; x++)
		    {
			if (opt[1] == arg[name,x])
			{
			    args[x] = opt[2];

			    if (index(flags[name,x],"M"))
			    {
				for (t++; t<=nargs; t++)
				{
				    args[x] = args[x] " " userargs[t];
				}
			    }

			    break;
			}
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
		str=body[name,t];
#printf ("Body[%d] = %s\n", t, body[name,t]);

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
