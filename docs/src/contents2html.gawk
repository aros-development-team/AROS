BEGIN {
    indent="";

    #for (t=0; t<ARGC; t++)
    #	 print t":"ARGV[t];

    if (ARGC > 1)
	proccontents(ARGV[1]);
}

function proccontents(file  ,t,dir,tmp_dir,text,desc) {
    desc=0;
    while ((getline < file) > 0)
    {
	if (match ($0,/^[-a-zA-Z_./]+[ \t]+-[ \t]+/))
	{
	    if (!desc)
	    {
		print indent"\\begin{description}"
		desc=1;
	    }

	    text=substr($0,RSTART+RLENGTH);
	    tmp_dir=$1;
	    if (dir != "")
	    {
		#print "Looking for "TOP"/"dir"contents"
		for (t=1; t<ARGC; t++)
		{
		    if (ARGV[t] == TOP"/"dir"contents")
		    {
			indent=indent"    "
			proccontents(ARGV[t]);
			indent=substr(indent,5);
		    }
		}
	    }

	    dir=tmp_dir;

	    print indent"\\item{"dir"} "text
	}
	else
	    print
    }
    print indent"\\end{description}\n"
}
