#!/usr/local/Hughes/bin/lite

$sock = msqlConnect("");

if ($sock < 0)
{
	echo ("Can't connect to server!\n");
	exit (10);
}

if (msqlSelectDB ($sock,"jobserv") < 0)
{
	echo ("Can't open database!\n");
	msqlClose ($sock);
	exit (10);
}

echo ("Content-type: text/html\n\n");
/* echo ("$argv[0] $argv[1]\n"); */

/* if (msqlQuery ($sock, "select jobid,comment from jobs where status = 0 and jobid like 'intu%'") < 0) */
if (msqlQuery ($sock, "select comment,email from jobs where status = 2") < 0)
{
	echo ("Error: $ERRMSG\n");
	msqlClose ($sock);
	exit (10);
}

$query = msqlStoreResult ();
printf ("There are %d completed jobs.<P>\n",
    msqlNumRows($query));
echo ("<TABLE>\n");
echo ("<TH>Job</TH><TH>Completed by</TH>\n");

$row = msqlFetchRow ($query);

if ( # $row == 0 )
{
    echo ("Error: $ERRMSG\n");
    msqlClose ($sock);
    exit (10);
}
else
{
    $col = 0;
    while ( # $row != 0 )
    {
	if ($col == 0)
	{
	    echo ("<TR>");
	}
	echo ("<TD>$row[0]</TD>");
	echo ("<TD><A HREF=\"mailto:$row[1]\">$row[1]</A></TD>\n");
	$row = msqlFetchRow ($query);
	$col = $col + 1;
	if ($col == 1)
	{
	    echo ("</TR>");
	    $col = 0;
	}
    }

    if ($col != 0)
    {
	echo ("</TR>");
    }
}
echo ("</TABLE>\n");

msqlFreeResult ($query);
msqlClose ($sock);
