BEGIN {
    file=ENVIRON["HOME"] "/Mail/jobs";

    while ((getline < file) > 0)
    {
	if (match($0,/^[a-zA-Z_/.]+[0-9]+ (WORK|DONE|FREE)/))
	{
	    match($0,/^[a-zA-Z_/.]+/);
	    name=substr($0,RSTART,RLENGTH);
	    job[name]++;
	    jobs ++;
	    if ($2 == "WORK")
	    {
		jobw[name]++;
		work ++;
	    }
	    else if ($2 == "DONE")
	    {
		jobd[name]++;
		done ++;
	    }
	    else
	    {
		jobf[name]++;
		free ++;
	    }
	}
	if (match($0,/^[a-zA-Z_/.]+ (WORK|DONE|FREE)/))
	{
	    ojobs ++;
	    if ($2 == "WORK") owork ++;
	    else if ($2 == "DONE") odone ++;
	    else ofree ++;
	}
    }

    close (file);

    print "There is a total of " jobs " functions."
    printf ("%4d (%7.2f%%) are still todo\n",        free, free*100.0/jobs);
    printf ("%4d (%7.2f%%) are currently in work\n", work, work*100.0/jobs);
    printf ("%4d (%7.2f%%) are completed\n",         done, done*100.0/jobs);
    print ""
    for (name in job)
    {
	if (jobf[name]!=job[name])
	{
	    if (jobd[name]!=job[name])
	    {
		printf ("%4d jobs in %s (%.2f%% todo, %.2f%% in work, %.2f%% completed\n",
		    job[name],
		    name,
		    jobf[name]*100.0/job[name],
		    jobw[name]*100.0/job[name],
		    jobd[name]*100.0/job[name]);
	    }
	    else
		printf ("%4d jobs in %s completed\n", job[name], name);
	}
    }
    print ""
    print "There is a total of " ojobs " other things."
    printf ("%4d (%7.2f%%) are still todo\n",        ofree, ofree*100.0/ojobs);
    printf ("%4d (%7.2f%%) are currently in work\n", owork, owork*100.0/ojobs);
    printf ("%4d (%7.2f%%) are completed\n",         odone, odone*100.0/ojobs);
}

