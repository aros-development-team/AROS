#!/usr/local/Hughes/bin/lite

printf ("Content-type: text/html\n\n");
/* echo ("$argv[0] $argv[1]\n"); */

printf ("<FORM ACTION=\"/~digulla/jobserver/cgi-bin/jobserv-work2.cgi\" METHOD=\"GET\">\n");
printf ("Identify yourself: <SELECT name=\"Name\">\n");

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

printf ("</SELECT><P>\n");

printf ("</TABLE>\n");
printf ("<INPUT TYPE=\"submit\" value=\"Login\"><P>\n");
printf ("</FORM>\n");
