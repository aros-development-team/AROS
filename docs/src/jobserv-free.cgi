#!/usr/local/Hughes/bin/lite

load "support.lib";

$sock = OpenDB ("aros.fh-konstanz.de", "jobserv");

if ($sock < 0)
{
    exit (10);
}

$login = getenv ("REMOTE_USER");
$name = getUserName ($sock, $login);

printf ("Content-type: text/html\n\n");

/* debugenv(0,$argc,$argv); */

$jobids="";

$query_string = getenv ("QUERY_STRING");

if ($query_string != "")
{
    $args = split ($query_string, "&");

    if (# $args > 0)
    {
	$query_string = "";

	$t = 0;

	while ($t < #$args)
	{
	    $area = split ($args[$t], "=");
	    $t = $t + 1;

	    if ($jobids != "")
	    {
		$jobids = $jobids + " or ";
	    }

	    $jobids = $jobids + "jobid like '" + $area[1] + "0%'";
	}
    }
}

if ($jobids != "")
{
    $jobids = "and (" + $jobids + ")";
}

/* printf ("%s<P>\n", $jobids); */

$res = msqlQuery ($sock, "select jobid,comment from jobs where status = 0 " +
	    $jobids + " order by comment");

if ($res < 0)
{
    echo ("Error: $ERRMSG\n");
    msqlClose ($sock);
    exit (10);
}

printf ("Welcome %s. There are %d jobs free. Select the ones\n",
    $name,
    $res);
printf ("your want to do and submit the form<P>\n");

$query = msqlStoreResult ();

echo ("<FORM ACTION=\"jobserv.cgi\" METHOD=\"GET\">\n");
echo ("<TABLE>\n");
echo ("<TR><TH>Req</TH><TH>Job</TH><TH>Req</TH><TH>Job</TH><TH>Req</TH><TH>Job</TH></TR>\n");

$row = msqlFetchRow ($query);

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

echo ("</TABLE>\n");
echo ("<INPUT TYPE=\"submit\" value=\"Allocate jobs\"><P>\n");
echo ("</FORM>\n");

msqlFreeResult ($query);
msqlClose ($sock);
