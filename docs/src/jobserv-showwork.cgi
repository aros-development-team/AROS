#!/usr/local/Hughes/bin/lite

$sock = msqlConnect("aros.fh-konstanz.de");

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

$res = msqlQuery ($sock, "select comment,email from jobs where status = 1 order by comment");

if ($res < 0)
{
	echo ("Error: $ERRMSG\n");
	msqlClose ($sock);
	exit (10);
}

printf ("There are %d jobs currently in work.<P>\n",
    $res);

$query = msqlStoreResult ();

echo ("<TABLE>\n");
echo ("<TR><TH>Job</TH><TH>Beeing processed by</TH></TR>\n");

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

echo ("</TABLE>\n");

msqlFreeResult ($query);
msqlClose ($sock);
