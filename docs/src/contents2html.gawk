BEGIN {
    indent="";
    stderr="/dev/stderr";

#    for (t=0; t<ARGC; t++)
#	 print t":"ARGV[t];

    proccontents(ARGV[1],TOP);
}

function proccontents(file,top  ,t,dir,tmp_dir,text,desc) {
    desc=0;
    print "Working on",file >> stderr;
    #relpath=file;
    #sub (TOP, "", relpath);
    #sub (/\/contents$/, relpath);

    # Read the file
    while ((getline < file) > 0)
    {
	# Is this an item (work blank - blank) ?
	if (match ($0,/^[-a-zA-Z0-9_./, ]+[ \t]+-[ \t]*/))
	{
	    if (!desc)
	    {
		print indent"\\begin{description}"
		desc=1;
	    }

	    # Find rest of the line
	    text=substr($0,RSTART+RLENGTH);
	    item=substr($0,RSTART,RLENGTH);
	    tmp_dir=$1;

	    if (match (dir, /\//))
	    {
		gsub("/$","",dir);
		print "Looking for "top"/"dir"/contents" >> stderr;
		indent=indent"    "
		proccontents(top"/"dir"/contents",top"/"dir);
		indent=substr(indent,5);
	    }

	    dir=tmp_dir;

	    # Remove blank-blank in item and print it.
	    sub(/[ \t]+-[ \t]+$/, "", item);
	    print indent"\\item{"item"} "text
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

    if (match (dir, /\//))
    {
	gsub("/$","",dir);
	indent=indent"    "
	proccontents(top"/"dir"/contents",top"/"dir);
	indent=substr(indent,5);
    }

    if (desc)
    {
	print indent"\\end{description}\n"
	desc=0;
    }
}

# vim:syn=awk:
