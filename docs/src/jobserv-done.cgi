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

$res = msqlQuery ($sock, "select comment from jobs where status = 2 and email = '" + $login + "' order by comment");

if ($res < 0)
{
    printf ("Error with query: %s\n", $ERRMSG);
    msqlClose ($sock);
    exit (10);
}

printf ("These %d jobs were finished by you:<P>\n", $res);

$query = msqlStoreResult ();

printf ("<TABLE>\n");
printf ("<TR><TH>Job</TH><TH>Job</TH><TH>Job</TH></TR>\n");

$row = msqlFetchRow ($query);

$col = 0;
while ( # $row != 0 )
{
    if ($col == 0)
    {
	printf ("<TR>");
    }
    printf ("<TD>%s</TD>\n", $row[0]);
    $row = msqlFetchRow ($query);
    $col = $col + 1;
    if ($col == 3)
    {
	printf ("</TR>");
	$col = 0;
    }
}

if ($col != 0)
{
    printf ("</TR>");
}

printf ("</TABLE>\n");

msqlFreeResult ($query);
msqlClose ($sock);
