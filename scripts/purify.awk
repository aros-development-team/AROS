BEGIN {
#    for (t=0; t<ARGC; t++)
#	 print t ": " ARGV[t];

    infile=ARGV[1];
    outfile=ARGV[3];
    emit_strings=0;

#    printf ("Reading %s\n", infile);

    while ((getline < infile) > 0)
    {
	if ($1==".file")
	{
	    file=$2
	    emit_strings=1;
	}
	else if ($1==".stabn")
	{
	    split($2,a,",");
	    line=a[3];
	}
	else if ($1=="call" && !match($2,/^\*\%/))
	{
	    strings[$2]=1;
	    emit_strings=1;
	}
	else if (match($0,/^.LC/))
	{
	    t=int(substr($0,4));
	    if (t>lc)
		lc=t;
	}
    }

    gsub(/"/,"",file);
    strings[file]=1;

    close (infile);
    lc++;

#    printf ("Writing %s\n", outfile);

    printf ("") > outfile;
    line=-1;

    while ((getline < infile) > 0)
    {
	if ($1==".text")
	{
	    if (emit_strings)
	    {
		print ".section\t.rodata" >> outfile
		for (str in strings)
		{
		    print ".LC" lc ":" >> outfile
		    print "\t.string \"" str "\"" >> outfile;
		    strings[str] = lc;
		    lc ++;
		}
		emit_strings=0;
	    }
	}
	else if ($1==".stabn")
	{
	    split($2,a,",");
	    line=a[3];
	}
	else if ($1=="call" && !match($2,/^\*\%/))
	{
	    if (line==-1)
		print "\tpushl\t$" NF  >> outfile;
	    else
		print "\tpushl\t$" line  >> outfile;
	    print "\tpushl\t$.LC" strings[file] >> outfile;
	    print "\tpushl\t$.LC" strings[$2] >> outfile;
	    print "\tcall\tRT_IntEnter" >> outfile;
	    print "\taddl\t$12,%esp" >> outfile;
	    strings[$2]=1;
	    #print "call to " $2 " at " file ":" line
	    print >> outfile
	    print "\tcall\tRT_Leave" >> outfile;
	    continue;
	}
	print >> outfile;

	if ($1=="main:")
	    print "\tcall\tRT_Init" >> outfile;
    }
}
