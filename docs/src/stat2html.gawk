BEGIN {
    FS=";";

    pos = 0;
    jobno = 0;
    job[""] = 0;

    file="abbreviations.dat";
    while ((getline < file) > 0)
    {
	long[$1] = $2;
    }
    close (file);

    file="jobs.dat";
    while ((getline < file) > 0)
    {
	jobid=$1;
	status=$2;
	login=$3;
	comment=$4;
	
	if (match (substr (jobid, length(jobid)-4), /^[0-9]+$/))
	{
	    name=substr(jobid,1,length(jobid)-5);
	    #print name
	    if (!(name in long))
	    {
		printf ("'%s' not found in list of abbreviations\n", name) >> "/dev/stderr";
		long[name] = name;
	    }

	    if (!(name in job) )
	    {
		jobname[jobno] = name;
		jobno ++;
	    }
	    
	    job[name]++;
	    jobs ++;
	    if (status==1)
	    {
		jobw[name]++;
		work ++;
	    }
	    else if (status==2)
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
	else
	{
	    ojobs ++;
	    if (status==1) owork ++;
	    else if (status==2) odone ++;
	    else ofree ++;
	}
    }

    close (file);

    if (!jobs)
    {
	print "No jobs found" > "/dev/stderr";
	exit (10);
    }

    print "There is a total of " jobs " functions.<BR>\n"
    printf ("%4d (%.2f%%) are still todo.<BR>\n",        free, free*100.0/jobs);
    printf ("%4d (%.2f%%) are currently in work.<BR>\n", work, work*100.0/jobs);
    printf ("%4d (%.2f%%) are completed.<P>\n",         done, done*100.0/jobs);

    print "<TABLE BORDER=1><TR><TH>Job</TH><TH>Count</TH><TH>Todo</TH><TH>In work</TH><TH>Completed</TH></TR>"
    for (t=0; t<jobno; t++)
    {
	name = jobname[t];

	if (job[name] != jobf[name])
	    printf ("<TR><TD>%s</TD><TD ALIGN=RIGHT>%d</TD><TD ALIGN=RIGHT>%.2f%%</TD><TD ALIGN=RIGHT>%.2f%%</TD><TD ALIGN=RIGHT>%.2f%%</TD></TR>\n",
		long[name],
		job[name],
		jobf[name]*100.0/job[name],
		jobw[name]*100.0/job[name],
		jobd[name]*100.0/job[name]);
    }
    print "</TABLE><P>"
    print "There is a total of " ojobs " other things.<BR>\n"
    printf ("%4d (%.2f%%) are still todo.<BR>\n",        ofree, ofree*100.0/ojobs);
    printf ("%4d (%.2f%%) are currently in work.<BR>\n", owork, owork*100.0/ojobs);
    printf ("%4d (%.2f%%) are completed.<P>\n",         odone, odone*100.0/ojobs);
}

