#!/usr/local/Hughes/bin/lite

load "support.lib";

$sock = OpenDB ("aros.fh-konstanz.de", "jobserv");

if ($sock < 0)
{
    exit (10);
}

printf ("Content-type: text/html\n\n");

debugenv(0,$argc,$argv);

$login=getenv("REMOTE_USER");
$userdata=getUserData($login);

$query_string = getenv ("QUERY_STRING");
$info = split ($query_string, "=");

printf ("query_string=%s<BR>\n", $query_string);

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
