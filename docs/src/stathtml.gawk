BEGIN {
    file="./jobserv-query.lite 'select jobid,status from jobs order by jobid'";

    file | getline cnt;
    pos = 0;
    jobno = 0;

    while ((file | getline) > 0)
    {
	if ($0 == "")
	{
	    pos = 0;
	    status = int(entry[1]);

	    if (match(entry[0],/^[a-zA-Z_]+[0-9]+$/))
	    {
		match(entry[0],/^[a-zA-Z_]+/);
		name=substr(entry[0],RSTART,RLENGTH);
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
	else
	{
	    entry[pos++] = substr($0,2);
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
		name,
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

