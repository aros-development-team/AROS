BEGIN {
    indent="";

#    for (t=0; t<ARGC; t++)
#	 print t":"ARGV[t];

    if (ARGC > 1)
	proccontents(ARGV[1],TOP);
}

function proccontents(file,top  ,t,dir,tmp_dir,text,desc) {
    desc=0;
#   print "Working on " top
    while ((getline < file) > 0)
    {
	if (match ($0,/^[-a-zA-Z0-9_./]+[ \t]+-[ \t]*/))
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
		gsub("/$","",dir);
		#print "Looking for "top"/"dir"/contents"
		for (t=1; t<ARGC; t++)
		{
		    if (ARGV[t] == top"/"dir"/contents")
		    {
			indent=indent"    "
			proccontents(ARGV[t],top"/"dir);
			indent=substr(indent,5);
		    }
		}
	    }

	    dir=tmp_dir;

	    print indent"\\item{"dir"} "text
	}
	else if (match ($0,/^[ \t]*\\filtermakefile{.*}/))
	{
	    args=substr($0,RSTART,RLENGTH);
	    pos=index(args,"{");
	    args=substr(args,pos+1,length(args)-pos-1);

	    narg=split(args,arg,",");

	    filename=arg[1];
	    gsub(/\$[(]TOP[)]/,TOP,filename);

	    cmd = "gawk -f makefile2html.gawk --assign secfilt=\""arg[2]"\" \""filename"\"";

	    #print "cmd=" cmd;

	    while ((cmd | getline) > 0)
		print;

	    close (cmd);
	}
	else
	    print
    }

    if (dir!="")
    {
	gsub("/$","",dir);
	for (t=1; t<ARGC; t++)
	{
	    if (ARGV[t] == top"/"dir"/contents")
	    {
		indent=indent"    "
		proccontents(ARGV[t],top"/"dir);
		indent=substr(indent,5);
	    }
	}
    }

    print indent"\\end{description}\n"
}
