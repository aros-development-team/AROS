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

$res = msqlQuery ($sock, "select jobid,comment from jobs where status = 1 and email = '" + $login + "' order by comment");

if ($res < 0)
{
    printf ("Error with query: %s\n", $ERRMSG);
    msqlClose ($sock);
    exit (10);
}

printf ("There are %d jobs allocated by you.\n", $res);

$query = msqlStoreResult ();
printf ("Now select the jobs you want to mark as completed or deallocate\n");
printf ("and submit the form<P>\n");

printf ("<FORM ACTION=\"/~digulla/jobserver/cgi-bin/jobserv.cgi\" METHOD=\"PUT\">\n");
printf ("<TABLE>\n");
printf ("<TR><TH>Free</TH><TH>Done</TH><TH>Job</TH><TH>Free</TH><TH>Done</TH><TH>Job</TH></TR>\n");

$row = msqlFetchRow ($query);

$col = 0;
while ( # $row != 0 )
{
    if ($col == 0)
    {
	printf ("<TR>");
    }
    printf ("<TD><INPUT TYPE=\"radio\" NAME=\"%s\" VALUE=\"free\"></TD><TD><INPUT TYPE=\"radio\" NAME=\"%s\" VALUE=\"done\"></TD><TD>%s</TD>\n",
	$row[0],
	$row[0],
	$row[1]
    );
    $row = msqlFetchRow ($query);
    $col = $col + 1;
    if ($col == 2)
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
printf ("<INPUT TYPE=\"submit\" value=\"Update\"><P>\n");
printf ("</FORM>\n");

msqlFreeResult ($query);
msqlClose ($sock);
