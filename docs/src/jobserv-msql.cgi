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
$query_string = getenv ("QUERY_STRING");
$args = split ($query_string, "=");
$query = urlDecode ($args[1]);

if (1 == 0)
{
    $fd = open ("id", "<|");
    $line = readln ($fd);
    printf ("You are: %s<P>\n", $line);
    close ($fd);
}

printf ("You have issued the following command:<P>\n", $res);

printf ("<PRE>%s</PRE><P>\n", $query);

$res = msqlQuery ($sock, $query);

if ($res < 0)
{
    printf ("Error with query: %s\n", $ERRMSG);
    msqlClose ($sock);
    exit (10);
}

$query = msqlStoreResult ();
$res = msqlNumRows ($query);
/* printf ("res=|%d|<P>\n", $res); */

if ($res == 0)
{
    printf ("No results.\n");
}
else
{
    $row = msqlFetchRow ($query);

    printf ("This is the result:<P>\n");

    printf ("<TABLE>\n");

    while ( # $row != 0 )
    {
	$t = 0;
	printf ("<TR>");
	while ($t < #$row)
	{
	    printf ("<TD>%s</TD>", $row[$t]);
	    $t = $t + 1;
	}
	printf ("</TR>\n");
	$row = msqlFetchRow ($query);
    }

    printf ("</TABLE>\n");
}

msqlFreeResult ($query);

msqlClose ($sock);
