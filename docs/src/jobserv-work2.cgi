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

/* printf ("%d<PRE>", $argc);
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
$info = split ($query_string, "=");

/* printf ("query_string=%s<BR>\n", $query_string); */

$email = urlDecode ($info[1]);

/* printf ("email=%s<BR>\n", $email); */

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

printf ("<FORM ACTION=\"/~digulla/jobserver/cgi-bin/jobserv.cgi\" METHOD=\"PUT\">\n");
printf ("EMail = <INPUT TYPE=\"text\" VALUE=\"%s\" NAME=\"email\" SIZE=50 MAXLENGTH=256><P>\n",
    $email);
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
