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

    basename_lp=indir "/clib/" tolower(lib) "_protos.h"
    basename_lc=indir "/defines/" tolower(lib) ".h"
    infile_lp=basename_lp
    infile_lc=basename_lc
    out_lp=basename_lp ".new"
    out_lc=basename_lc ".new"

    todo=2;

    printf ("") > out_lp;
    printf ("") > out_lc;

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

    todo=2;

    while ((getline < infile_lc) > 0 && todo)
    {
	if ($1 == "Defines")
	    todo --;
	else if ($1 == "#ifndef" && define_lc=="")
	{
	    define_lc = $2;
	}
	else if (todo < 2)
	    todo --;

	print >> out_lc;
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
    gsub("AROS_LH","AROS_LP",line);
    print line >> out_lp;
    print "" >> out_lp;

    printf ("#define %s(", name) >> out_lc;
    for (t=0; t<args; t++)
    {
	printf ("%s", a[t]) >> out_lc;

	if (t+1<args)
	    printf (", ") >> out_lc;
    }
    printf (") \\\n") >> out_lc
    line=f;
    gsub("@1"," \\\n    ",line);
    gsub("AROS_LH","AROS_LC",line);
    print "    " line >> out_lc;
    print "" >> out_lc;
}
END {
    print "\n#endif /* " define_lc " */" >> out_lc
    print "\n#endif /* " define_lp " */" >> out_lp

    close (infile_lc);
    close (out_lc);
    close (infile_lp);
    close (out_lp);
}
