#NAME */
#	 #include <clib/exec_protos.h>
#
#	 __AROS_LH3(void, InitStruct,
#
#/*  SYNOPSIS */
#	 __AROS_LHA(APTR,  initTable, A1),
#	 __AROS_LHA(APTR,  memory,    A2),
#	 __AROS_LHA(ULONG, size,      D0),
#
#/*  LOCATION */
#	 struct ExecBase *, SysBase, 13, Exec)
#
#
#LP3(void, InitStruct,
#    APTR, initTable, A1,
#    APTR, memory, A2,
#    ULONG, size, D0,
#    struct ExecBase *, SysBase, -13, Exec)
#
##define InitStruct(initTable, memory, size) \
#__AROS_LC3(void, InitStruct, \
# __AROS_LA(APTR,  initTable, A1), \
# __AROS_LA(APTR,  memory,    A2), \
# __AROS_LA(ULONG, size,      D0), \
#	    struct ExecBase *, SysBase, 13, Exec)
#

BEGIN {
    indir="compiler/include"
    outdir=indir;

    infile_lp=pubclib
    infile_ld=pubdef
    pinfile_lp=privclib
    pinfile_ld=privdef
    out_lp=infile_lp ".new";
    out_ld=infile_ld ".new";
    pout_lp=pinfile_lp ".new";
    pout_ld=pinfile_ld ".new";

    printf ("") > out_lp;
    printf ("") > out_ld;
    printf ("") > pout_lp;
    printf ("") > pout_ld;

    todo=2;

    while ((getline < infile_lp) > 0 && todo)
    {
	if ($1 == "Prototypes")
	    todo --;
	else if ($1 == "#ifndef" && define_lp=="")
	{
	    define_lp = $2;
	}
	else if (todo < 2)
	    todo --;

	print >> out_lp;
    }

    close (infile_lp);

    todo=2;

    while ((getline < infile_ld) > 0 && todo)
    {
	if ($1 == "Defines")
	    todo --;
	else if ($1 == "#ifndef" && define_ld=="")
	{
	    define_ld = $2;
	}
	else if (todo < 2)
	    todo --;

	print >> out_ld;
    }

    close (infile_ld);

    todo=2;

    while ((getline < pinfile_lp) > 0 && todo)
    {
	if ($1 == "Prototypes")
	    todo --;
	else if ($1 == "#ifndef" && pdefine_lp=="")
	{
	    pdefine_lp = $2;
	}
	else if (todo < 2)
	    todo --;

	print >> pout_lp;
    }

    close (pinfile_lp);

    todo=2;

    while ((getline < pinfile_ld) > 0 && todo)
    {
	if ($1 == "Defines")
	    todo --;
	else if ($1 == "#ifndef" && pdefine_ld=="")
	{
	    pdefine_ld = $2;
	}
	else if (todo < 2)
	    todo --;

	print >> pout_ld;
    }

    close (pinfile_ld);
}
/^\/\*\*\*\*\*+/ {
    if (match($0,/\*\*i\*/))
    {
#	 printf ("%s is private\n", FILENAME);
	public=0;
    }
    else
    {
#	 printf ("%s is %s\n", FILENAME, "public");
	public=1;
    }
}
/^#?[ \t]*NAME[ \t]*(\*\/)?[ \t]*$/ {
    state=0;
    args=0;

    while (getline > 0)
    {
	if (state==0)
	{
	    if (match ($0,"^#?[ \t]*AROS_LH"))
	    {
		line=$0;
		sub(/#?[ \t]*$/,"",line);
		match(line,/[a-zA-Z0-9_]+,$/);
		name=substr(line,RSTART,RLENGTH-1);

		sub(/^#?[ \t]*/,"",line);
		f=line;
		state=1;
	    }
	}
	else if (state == 1)
	{
	    if (match ($0,"^#?[ \t]*AROS_LH"))
	    {
		line=$0;
		sub(/[ \t]*$/,"",line);
		sub(/^#?[ \t]*/,"",line);
		f=f "@1" line;

		match(line,/,[ \t]*[a-zA-Z0-9_]+[ \t]*,/);
		arg=substr(line,RSTART+1,RLENGTH-2);
		sub(/[ \t]*$/,"",arg);
		sub(/^[ \t]*/,"",arg);
		a[args++] = arg;
#print name ":" arg
	    }
	    else if (match ($0, /^#?(\/\*)?[ \t]*LOCATION[ \t]*(\*\/)?[ \t]*$/))
	    {
		state=2;
	    }
	}
	else if (state == 2)
	{
	    if ($1 != "")
	    {
		if (!match ($0,lib))
		    next;

		line=$0;
		sub(/[ \t]*$/,"",line);
		sub(/^#?[ \t]*/,"",line);
		f=f "@1" line;
		break;
	    }
	}
    }

    if (public)
    {
	outp = out_lp;
	outd = out_ld;
    }
    else
    {
	outp = pout_lp;
	outd = pout_ld;
    }

    line=f;
    gsub("@1","\n    ",line);
    gsub("AROS_LH","AROS_LP",line);
    print line >> outp;
    print "" >> outp;

    printf ("#define %s(", name) >> outd;
    for (t=0; t<args; t++)
    {
	printf ("%s", a[t]) >> outd;

	if (t+1<args)
	    printf (", ") >> outd;
    }
    printf (") \\\n") >> outd
    line=f;
    gsub("@1"," \\\n    ",line);
    gsub("AROS_LH","AROS_LC",line);
    print "    " line >> outd;
    print "" >> outd;
}
END {
    print "\n#endif /* " define_ld " */" >> out_ld
    print "\n#endif /* " define_lp " */" >> out_lp
    if (pdefine_ld != "")
	print "\n#endif /* " pdefine_ld " */" >> pout_ld
    if (pdefine_lp != "")
	print "\n#endif /* " pdefine_lp " */" >> pout_lp

    close (out_ld);
    close (out_lp);
    close (pout_ld);
    close (pout_lp);
}
