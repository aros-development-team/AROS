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
if (msqlQuery ($sock, "select jobid,comment from jobs where status = 0") < 0)
{
	echo ("Error: $ERRMSG\n");
	msqlClose ($sock);
	exit (10);
}

$query = msqlStoreResult ();
printf ("There are %d jobs free. Enter your EMail and select the ones\n",
    msqlNumRows($query));
echo ("your want to do and submit the form<P>\n");
echo ("<FORM ACTION=\"cgi-bin/jobs-alloc.cgi\" METHOD=\"GET\">\n");
echo ("<INPUT TYPE=\"text\" NAME=\"EMail\" SIZE=50 MAXLENGTH=256><P>\n");
echo ("<TABLE>\n");

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
	echo ("<TD><INPUT TYPE=\"checkbox\" VALUE=\"$row[0]\"</TD>");
	echo ("<TD>$row[1]</TD>\n");
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
