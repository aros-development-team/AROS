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
    indir="include/clib/"
    outdir=indir;

    infile=indir tolower(lib) "_protos."

    system ("mv " infile "h " infile "bak");

    infile=indir tolower(lib) "_protos.bak"
    out=outdir tolower(lib) "_protos.h"

    todo=2;

    printf ("") > out;

    while ((getline < infile) > 0 && todo)
    {
	if ($1 == "Prototypes")
	    todo --;
	else if ($1 == "#ifndef" && define=="")
	{
	    define = $2;
	}
	else if (todo < 2)
	    todo --;

	print >> out;
    }
}
/^#?[ \t]*NAME[ \t]*(\*\/)?[ \t]*$/ {
    state=0;
    args=0;

    while (getline > 0)
    {
	if (state==0)
	{
	    if (match ($0,"^#?[ \t]*__AROS_LH"))
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
	    if (match ($0,"^#?[ \t]*__AROS_LH"))
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
		line=$0;
		sub(/[ \t]*$/,"",line);
		sub(/^#?[ \t]*/,"",line);
		f=f "@1" line;
		break;
	    }
	}
    }

    line=f;
    gsub("@1","\n    ",line);
    gsub("__AROS_LH","__AROS_LP",line);
    print line >> out;

    printf ("#define %s(", name) >> out;
    for (t=0; t<args; t++)
    {
	printf ("%s", a[t]) >> out;

	if (t+1<args)
	    printf (", ") >> out;
    }
    printf (") \\\n") >> out
    line=f;
    gsub("@1"," \\\n    ",line);
    gsub("__AROS_LH","__AROS_LC",line);
    print "    " line >> out;
    print "" >> out;
}
END {
    print "\n#endif /* " define " */" >> out
}
