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
	if (match($0,/[a-zA-Z_]+ (WORK|DONE|FREE)/))
	{
	    ojobs ++;
	    if ($2 == "WORK") owork ++;
	    else if ($2 == "DONE") odone ++;
	    else ofree ++;
	}
    }

    print "There is a total of " jobs " functions."
    printf ("%4d (%7.2f%%) are still todo\n",        free, free*100.0/jobs);
    printf ("%4d (%7.2f%%) are currently in work\n", work, work*100.0/jobs);
    printf ("%4d (%7.2f%%) are completed\n",         done, done*100.0/jobs);
    print ""
    print "There is a total of " ojobs " other things."
    printf ("%4d (%7.2f%%) are still todo\n",        ofree, ofree*100.0/ojobs);
    printf ("%4d (%7.2f%%) are currently in work\n", owork, owork*100.0/ojobs);
    printf ("%4d (%7.2f%%) are completed\n",         odone, odone*100.0/ojobs);
}

