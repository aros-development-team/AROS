BEGIN {
    file=ENVIRON["HOME"] "/Mail/jobs";

    while ((getline < file) > 0)
    {
	if (match($0,/[a-zA-Z_]+[0-9]+ (WORK|DONE|FREE)/))
	{
	    jobs ++;
	    if ($2 == "WORK") work ++;
	    else if ($2 == "DONE") done ++;
	    else free ++;
	}
    }

    print "There is a total of " jobs " functions."
    printf ("%4d (%7.2f%%) are still todo\n", free, free*100.0/jobs);
    printf ("%4d (%7.2f%%) are currently in work\n", work, work*100.0/jobs);
    printf ("%4d (%7.2f%%) are completed\n", done, done*100.0/jobs);
}

