BEGIN {
    maxlvo = 0;
    stderr="/dev/stderr";

    print "/*";
    print "    (C) 1995-96 AROS - The Amiga Replacement OS";
    print "    *** Automatic generated file. Do not edit ***";
    print "    Desc: Function table for " lib;
    print "    Lang: english";
    print "*/";
    print "#ifndef LIBCORE_COMPILER_H\n#   include <libcore/compiler.h>\n#endif";
    print "#ifndef NULL\n#define NULL ((void *)0)\n#endif\n";
    print "#include \"libdefs.h\"";

    f[1] = "open";
    f[2] = "close";
    f[3] = "expunge";
    f[4] = "null";

    if (maxlvo < 4)
	maxlvo = 4;
}
/AROS_LH[0-9]/ {
    line=$0;
    sub(/[ \t]*$/,"",line);
    if (match(line,/[a-zA-Z0-9_]+,$/))
    {
	name=substr(line,RSTART,RLENGTH-1);
    }
}
/LIBBASE[ \t]*,[ \t]*[0-9]+/ {
    match ($0, /LIBBASE[ \t]*,[ \t]*[0-9]+/);
    line=substr($0,RSTART,RLENGTH);
    sub (/LIBBASE[ \t]*,[ \t]*/,"",line);
    lvo=int(line);

    if (f[lvo] != "")
	printf "Error: lvo "lvo" is used by "f[lvo]" and "name >> stderr;

    f[lvo] = name;
    if (lvo > maxlvo)
	maxlvo = lvo;
}
/^\/\*AROS/ {
    if ($2 == "alias") {
	a[lvo] = $3;
	f[lvo] = $4;
    }
}
END {
    print "extern void AROS_SLIB_ENTRY(OpenLib,LibHeader) (void);";
    print "extern void AROS_SLIB_ENTRY(CloseLib,LibHeader) (void);";
    print "extern void AROS_SLIB_ENTRY(ExpungeLib,LibHeader) (void);";
    print "extern void AROS_SLIB_ENTRY(ExtFuncLib,LibHeader) (void);";

    for (t=5; t<=maxlvo; t++)
    {
	if (t in f && !(t in a))
	    print "extern void AROS_SLIB_ENTRY(" f[t] ",BASENAME) (void);";
    }

    print "\nvoid *const LIBFUNCTABLE[]=\n{";

    show=0;

    print "    AROS_SLIB_ENTRY(OpenLib,LibHeader),";
    print "    AROS_SLIB_ENTRY(CloseLib,LibHeader),";
    print "    AROS_SLIB_ENTRY(ExpungeLib,LibHeader),";
    print "    AROS_SLIB_ENTRY(ExtFuncLib,LibHeader),";

    for (t=5; t<=maxlvo; t++)
    {
	line="    ";

	if (t in f)
	{
	    line=line "AROS_SLIB_ENTRY(" f[t] ",BASENAME),";

	    if (t in a)
		line=line " /* " a[t] " " t " */";
	    else
		line=line " /* " t " */";
	}
	else
	    line=line "NULL, /* " t " */";

	print line;

	show=0;
    }

    print "    (void *)-1L";
    print "};";
}

