#!/usr/local/Hughes/bin/lite

$sock = OpenDB ("aros.fh-konstanz.de", "jobserv");

if ($sock < 0)
{
	exit (10);
}

echo ("Content-type: text/html\n\n");
/* echo ("$argv[0] $argv[1]\n"); */

/* if (msqlQuery ($sock, "select jobid,comment from jobs where status = 0 and jobid like 'intu%'") < 0) */
/* $res = msqlQuery ($sock, "select comment,email from jobs where status = 2"); */
/* $res = msqlQuery ($sock, "select comment,email from jobs where status = 2 and jobid like 'intu%' order by comment"); */
$res = msqlQuery ($sock, "select comment,email from jobs where status = 2 order by comment");

if ($res < 0)
{
	echo ("Error: $ERRMSG\n");
	msqlClose ($sock);
	exit (10);
}

printf ("There are %d completed jobs.<P>\n", $res);

$query = msqlStoreResult ();
echo ("<TABLE>\n");
echo ("<TH>Job</TH><TH>Completed by</TH>\n");

$row = msqlFetchRow ($query);

$col = 0;
while ( # $row != 0 )
{
    if ($col == 0)
    {
	echo ("<TR>");
    }
    printf ("<TD>%s</TD><TD><A HREF=\"mailto:%s\">%s</A></TD>\n",
	$row[0],
	$row[1],
	$row[1]
    );
    /* echo ("<TD>$row[0]</TD>");
    echo ("<TD><A HREF=\"mailto:$row[1]\">$row[1]</A></TD>\n"); */
    $col = $col + 1;
    if ($col == 1)
    {
	echo ("</TR>\n");
	$col = 0;
    }
    $row = msqlFetchRow ($query);
    /* printf ("%d %s\n", # $row, $row[0]); */
}

if ($col != 0)
{
    echo ("</TR>");
}

echo ("</TABLE>\n");

msqlFreeResult ($query);
msqlClose ($sock);
