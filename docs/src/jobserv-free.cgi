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

$res = msqlQuery ($sock, "select jobid,comment from jobs where status = 0 order by comment");

if ($res < 0)
{
	echo ("Error: $ERRMSG\n");
	msqlClose ($sock);
	exit (10);
}

printf ("There are %d jobs free. Enter your EMail and select the ones\n",
    $res);

$query = msqlStoreResult ();
echo ("your want to do and submit the form<P>\n");
echo ("<FORM ACTION=\"cgi-bin/jobserv.cgi\" METHOD=\"GET\">\n");

$filename = "../devlist.txt";
/* printf ("filename=%s<P>\n", $filename); */

$fd = open ($filename, "<");
/* printf ("fd=%d<P>\n", $fd); */

if ($fd == -1)
{
    echo ("Error opening $filename: $ERRMSG\n");
    msqlClose ($sock);
    exit (10);
}

$line = readln ($fd);

echo ("<FORM ACTION=\"cgi-bin/jobserv.cgi\" METHOD=\"GET\">\n");
echo ("<SELECT name=\"email\">\n");

while ($line != "")
{
    $line = chop ($line);

    if ($line != "")
    {
	$info = split ($line, ":");

	printf ("<OPTION value=\"%s\">%s</OPTION>\n", $info[1], $info[0]);
    }

    $line = readln ($fd);
}

close ($fd);

echo ("</SELECT><P>\n");

echo ("<TABLE>\n");
echo ("<TR><TH>Req</TH><TH>Job</TH><TH>Req</TH><TH>Job</TH><TH>Req</TH><TH>Job</TH></TR>\n");

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
	printf ("<TD><INPUT TYPE=\"checkbox\" NAME=\"%s\" VALUE=\"req\"></TD><TD>%s</TD>\n",
	    $row[0],
	    $row[1]
	);
	$row = msqlFetchRow ($query);
	$col = $col + 1;
	if ($col == 3)
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
echo ("<INPUT TYPE=\"submit\" value=\"Allocate jobs\"><P>\n");
echo ("</FORM>\n");

msqlFreeResult ($query);
msqlClose ($sock);
