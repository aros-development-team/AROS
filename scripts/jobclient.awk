BEGIN {
    file=ENVIRON["HOME"] "/Mail/jobs"

    while ((getline < file) > 0)
    {
	if (match($0,/^#/) || $1=="")
	    continue;

	id=$1; status=$2;
	getline < file;
	email=$1;
	text="";
	while ((getline line < file) > 0)
	{
	    if (line=="")
		break;

	    if (text=="")
		text=line;
	    else
		text=text "\n" line;
	}

	for (t=0; t<ARGC; t++)
	{
	    if (match (text,ARGV[t]))
	    {
		gsub(text,"\n","\n# ");
		print "# " text;
		print "req " id "\ndone " id;
		break;
	    }
	}
    }
}
