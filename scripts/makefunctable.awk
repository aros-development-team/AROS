BEGIN {
    maxlvo = 0;

    oname=tolower(lib) "_functable.c";

    print "/*" > oname;
    print "        (C) 1995-96 AROS - The Amiga Replacement OS" >> oname
    print "        *** Automatic generated file. Do not edit ***" >> oname
    print "        Desc: Funktion table for " lib >> oname
    print "        Lang: english" >> oname
    print "*/" >> oname;
    print "#ifndef AROS_LIBCALL_H\n#   include <aros/libcall.h>\n#endif" >> oname;
    print "#ifndef NULL\n#define NULL ((void *)0)\n#endif\n" >> oname;

    f[1] = "open";
    f[2] = "close";
    f[3] = "expunge";
    f[4] = "null";

    if (maxlvo < 4)
	maxlvo = 4;
}
/^[ \t]*NAME[ \t]*(\*\/)?[ \t]*$/ {
	while (getline > 0)
	{
	    if (match($0,/^[ \t]*__AROS_LH/))
	    {
		line=$0;
		sub(/[ \t]*$/,"",line);
		match(line,/[a-zA-Z0-9_]+,$/);
		name=substr(line,RSTART,RLENGTH-1);
		break;
	    }
	}
    }
/^(\/\*)?[ \t]*LOCATION[ \t]*(\*\/)?[ \t]*$/ {
    while (getline > 0)
    {
	if (match ($0, /[0-9]+,/))
	{
	    lvo=int(substr($0,RSTART,RLENGTH-1));

	    f[lvo] = name;
	    if (lvo > maxlvo)
		maxlvo = lvo;

	    break;
	}

	if (match ($0, /(\*\/|\*\/)/))
	    break;
    }
}
/^\/\*AROS/ {
    if ($2 == "alias") {
	a[lvo] = $3;
	f[lvo] = $4;
    }
}
END {
    for (t=1; t<=maxlvo; t++)
    {
	if (t in f && !(t in a))
	    print "void __AROS_SLIB_ENTRY(" f[t] "," lib ") (void);" >> oname;
    }

    print "\nvoid *const " lib "_functable[]=\n{" >> oname;

    show=0;

    for (t=1; t<=maxlvo; t++)
    {
	line="    ";

	if (t in f)
	{
	    line=line "__AROS_SLIB_ENTRY(" f[t] "," lib "),";

	    if (t in a)
		line=line " /* " a[t] " " t " */";
	    else
		line=line " /* " t " */";
	}
	else
	    line=line "NULL, /* " t " */";

	print line >> oname;

	show=0;
    }

    print "    (void *)-1L" >> oname;
    print "};" >> oname;

    print "\nchar " lib "_end;" >> oname;
}

