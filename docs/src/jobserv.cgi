#!/usr/local/Hughes/bin/lite

load "support.lib";

$sock = msqlConnect("aros.fh-konstanz.de");

if ($sock < 0)
{
    printf ("Can't connect to server!\n");
    exit (10);
}

if (msqlSelectDB ($sock,"jobserv") < 0)
{
    printf ("Can't open database!\n");
    msqlClose ($sock);
    exit (10);
}

printf ("Content-type: text/html\n\n");

/* printf ("<PRE>");
printf ("argc=%d\n", $argc);
$t = 0;
while ($t < $argc)
{
    printf ("%d: %s\n", $t, $argv[$t]);
    $t = $t + 1;
}

system ("printenv");
$line = read ($stdin,10);
if ($line == "")
{
    if ($ERRMSG != "")
    {
	printf ("Error reading stdin: %s\n", $ERRMSG);
    }
    else
    {
	printf ("EOF");
    }
}
else
{
    printf ("stdin = %s\n", $line);
}
printf ("</PRE>"); */

$query_string = getenv ("QUERY_STRING");

printf ("query_string=%s<BR>\n", $query_string);

$args = split ($query_string, "&");
$query_string = "";

$t = 0;

while ($t < #$args)
{
    $info = split ($args[$t], "=");
    $t = $t + 1;

    if ($info[0] == "email")
    {
	$email = urlDecode ($info[1]);
	printf ("Your EMail is %s<P>\n", $email);
    }
    else
    {
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
	    printf ("Allocating \"%s\"<BR>\n", $comment);
	    $status = 1;
	}
	else
	{
	    if ($info[1] == "free")
	    {
		printf ("Freeing \"%s\"<BR>\n", $comment);
		$status = 0;
	    }
	    else
	    {
		printf ("Done with \"%s\"<BR>\n", $comment);
		$status = 2;
	    }
	}

	$query = "update jobs set status=" +
		(char)$status +
		",email='" + $email + "' where jobid = '" +
		$info[0] + "'";

	$res = msqlQuery ($sock, $query);

	if ($res < 0)
	{
	    printf ("Error with query \"%s\": %s<BR>\n", $query, $ERRMSG);
	}
    }
}

/*
$res = msqlQuery ($sock, "select jobid,comment from jobs where status = 1 and email = '" + $email + "' order by comment");


printf ("There are %d jobs allocated by you.\n",
    $res);

$query = msqlStoreResult ();
$row = msqlFetchRow ($query);
msqlFreeResult ($query); */

msqlClose ($sock);
