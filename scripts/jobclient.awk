BEGIN {
    file=ENVIRON["HOME"] "/Mail/jobs"
    IGNORECASE=1

    while ((getline < file) > 0)
    {
	# Comment or empty line
	if (match($0,/^#/) || $1=="")
	    continue;

	for (t=0; t<ARGC; t++)
	{
	    # Does this job match any of the ones we look for ?
	    if (match ($0,ARGV[t]))
	    {
		# Job is not completed yet ?
		if ($2!="DONE")
		{
		    # Show description of job
		    if (NF > 3)
		    {
			printf ("# ");
			for (t=4; t<=NF; t++)
			    printf ("%s ", $t);
			print ""
		    }

		    # Show what can be done with the job
		    if ($2=="FREE")
			print "req "$1;
		    if ($2=="WORK" || $2=="FREE")
			print "done "$1;
		}

		# Don't try to print the job twice
		break;
	    }
	}
    }
}
