BEGIN {
    file=ENVIRON["HOME"] "/Mail/jobs"
    IGNORECASE=1

    while ((getline < file) > 0)
    {
	if (match($0,/^#/) || $1=="")
	    continue;

	if (!match ($0,/[a-zA-Z_]+[0-9]+ (FREE|WORK|DONE)/))
	    continue;

	id=$1; status=$2;
	getline < file;
	email=$1;
	getline text < file;

	for (t=0; t<ARGC; t++)
	{
	    if (match (id,ARGV[t]) || match (text,ARGV[t]))
	    {
		gsub(text,"\n","\n# ");
		print "# " text;
		if (status=="FREE")
		    print "req " id;
		if (status=="WORK" || status=="FREE")
		    print "done " id;
		break;
	    }
	}
    }
}
