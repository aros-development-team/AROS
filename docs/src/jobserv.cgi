#!/usr/local/Hughes/bin/lite

load "support.lib";

$sock = OpenDB ("aros.fh-konstanz.de", "jobserv");

if ($sock < 0)
{
    exit (10);
}

printf ("Content-type: text/html\n\n");

/* debugenv (0, $argc, $argv); */

$query_string = getenv ("QUERY_STRING");
$login = getenv ("REMOTE_USER");

/* printf ("query_string=%s<BR>\n", $query_string); */

if ($query_string != "")
{
    $args = split ($query_string, "&");

    if (# $args > 0)
    {
	$query_string = "";

	$t = 0;

	while ($t < #$args)
	{
	    $info = split ($args[$t], "=");
	    $t = $t + 1;

	    $jobid = urlDecode ($info[0]);

	    $query = "select comment from jobs where jobid = '" + $jobid + "'";

	    $res = msqlQuery ($sock, $query);

	    if ($res < 0)
	    {
		printf ("Error with query \"%s\": %s<BR>\n", $query, $ERRMSG);
		$comment = $info[0];
	    }
	    else
	    {
		$query = msqlStoreResult ();
		$row = msqlFetchRow ($query);
		msqlFreeResult ($query);
		$comment = $row[0];
	    }

	    if ($info[1] == "req")
	    {
		printf ("\"%s\" is now allocated by you.<BR>\n", $comment);
		$status = 1;
	    }
	    else
	    {
		if ($info[1] == "free")
		{
		    printf ("\"%s\" is no longer allocated by you.<BR>\n", $comment);
		    $status = 0;
		}
		else
		{
		    printf ("\"%s\" is marked as completed.<BR>\n", $comment);
		    $status = 2;
		}
	    }

	    $query = "update jobs set status=" +
		    (char)$status +
		    ",email='" + $login + "' where jobid = '" +
		    $info[0] + "'";

	    $res = msqlQuery ($sock, $query);

	    if ($res < 0)
	    {
		printf ("Error with query \"%s\": %s<BR>\n", $query, $ERRMSG);
	    }
	}
    }
}
else
{
    printf ("Nothing to be done.\n");
}

msqlClose ($sock);
