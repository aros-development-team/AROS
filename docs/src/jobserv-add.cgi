#!/usr/local/Hughes/bin/lite

load "support.lib";

$sock = OpenDB ("aros.fh-konstanz.de", "jobserv");

if ($sock < 0)
{
    exit (10);
}

printf ("Content-type: text/html\n\n");

/* debugenv(0,$argc,$argv); */

$login=getenv("REMOTE_USER");
$name=getUserName($sock,$login);

$query_string = getenv ("QUERY_STRING");
$args = split ($query_string, "&");

printf ("query_string=%s<BR>\n", $query_string);

$t = 0;
while ($t < #$args)
{
    $jobid = split ($args[$t],"=");
    $jobid = urlDecode ($jobid[1]);
    $t = $t + 1;
    $comment = split ($args[$t],"=");
    $comment = urlDecode ($comment[1]);
    $t = $t + 1;

    printf ("id=%s co=%s<BR>\n", $jobid, $comment);

    if ($jobid != "" && $comment != "")
    {
	$query = "INSERT INTO jobs (jobid,status,email,comment) VALUES ('" +
	    $jobid +
	    "',0,'" +
	    $login +
	    "','" +
	    $comment +
	    "')";
	$res = msqlQuery ($sock, $query);

	if ($res <= 0)
	{
	    printf ("Unable to add job %s (comment %s) to database: %s<BR>\n",
		$jobid, $comment, $ERRMSG);
	}
	else
	{
	    printf ("Added job %s (comment %s) to database.<BR>\n",
		$jobid, $comment);
	}
    }
    else
    {
	if ($jobid != "")
	{
	    printf ("You must specify a comment for %s\n", $jobid);
	}
	if ($comment != "")
	{
	    printf ("You must specify a jobid for %s\n", $comment);
	}
    }
}


/*
$res = msqlQuery ($sock, "select jobid,comment from jobs where status = 1 and email = '" + $email + "' order by comment");

if ($res < 0)
{
    printf ("Error with query: %s\n", $ERRMSG);
    msqlClose ($sock);
    exit (10);
}

printf ("There are %d jobs allocated by you.\n",
    $res);

$query = msqlStoreResult ();
printf ("Now select the jobs you want to mark as completed or deallocate\n");
printf ("and submit the form<P>\n");

$row = msqlFetchRow ($query);

while ( # $row != 0 )
{
    $row = msqlFetchRow ($query);
}

msqlFreeResult ($query); */
msqlClose ($sock);
