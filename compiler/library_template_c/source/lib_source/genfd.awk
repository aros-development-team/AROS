BEGIN {
    maxlvo = 0;
    stderr="/dev/stderr";

    file = "libdefs.h";

    while ((getline < file) > 0)
    {
	if ($2 == "BASENAME")
	{
	    lib = $3;
	    basename = $3;
	}
	else if ($2 == "LIBBASE")
	{
	    libbase = $3;
	}
	else if ($2 == "NT_TYPE")
	{
	    if ($3 == "NT_RESOURCE")
	    {
		firstlvo = 0;
	    }
	    else if ($3 == "NT_DEVICE")
	    {
		firstlvo = 6;
	    }
	    else
	    {
		firstlvo = 4;
	    }
	}
    }

    # Only for non-resources
    if( firstlvo >= 4 )
    {
	f[1] = "open";
	f[2] = "close";
	f[3] = "expunge";
	f[4] = "null";
    }
    verbose_pattern = libbase"[ \\t]*,[ \\t]*[0-9]+[ \\t]*,[ \\t]*"basename;

#print verbose_pattern > "/dev/stderr";

    if (maxlvo < firstlvo)
	maxlvo = firstlvo;
}
/AROS_LH(QUAD)?[0-9]/ {
    line=$0;
    sub(/[ \t]*$/,"",line);
    if (match(line,/[a-zA-Z0-9_]+,$/))
    {
	name=substr(line,RSTART,RLENGTH-1);
	args="";
	regs="";
#print "/* FOUND " name " */";
    }
}
/AROS_LHA/ {
    if (match($0,/,[^)]+/))
    {
	line=substr($0,RSTART+1,RLENGTH-1);
	gsub(/[ \t]+/,"",line);
	match(line,/[^,]+/);
	if (args!="")
	    args=args","substr(line,RSTART,RLENGTH);
	else
	    args=substr(line,RSTART,RLENGTH);
	if (regs!="")
	    regs=regs","substr(line,RSTART+RLENGTH+1);
	else
	    regs=substr(line,RSTART+RLENGTH+1);
    }
}
/LIBBASE[ \t]*,[ \t]*[0-9]+/ || $0 ~ verbose_pattern {
#print "/* LOC " $0 " */"
    match ($0, /,[ \t]*[0-9]+/);
    line=substr($0,RSTART,RLENGTH);
#print "line2="line
    sub (/,[ \t]*/,"",line);
#print "line3="line
    lvo=int(line);

    if (lvo == 0)
	print "Illegal LVO 0 in "FN
    else if (lvo <= firstlvo)
	f[lvo] = "";

#print "lvo="lvo;

    if (f[lvo] != "")
	print "Error: lvo "lvo" is used by "f[lvo]" and "name" in "FN >> stderr;

    f[lvo] = name;
    if (lvo > maxlvo)
	maxlvo = lvo;
    a_args[lvo] = args;
    gsub(/,/,"/",regs);
    a_regs[lvo] = tolower(regs);
}
/^\/\*AROS/ {
    if ($2 == "alias") {
	a[lvo] = $3;
	f[lvo] = $4;
    }
}
END {
    print "##base _"basename"Base";
    print "##bias " ((firstlvo+1)*6)
    print "##public"

    if (maxlvo <= firstlvo)
    {
	print "Error: No matching functions found" > "/dev/stderr";
	exit (10);
    }

    for (t=(firstlvo+1); t<=maxlvo; t++)
    {
	if (t in f)
	    print f[t]"("a_args[t]")("a_regs[t]")"
	else
	    print lib"Private"t"()()"
    }

    print "##end";
}

