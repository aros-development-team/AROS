#!/bin/sh

echo "Content-type: text/html"
echo ""
echo "<HTML>"
echo "<TITLE>AROS - Jobserver output</TITLE>"
echo "<H1>Output of the jobserver for your submission</H1>"
#echo "pwd"
#echo "<PRE>"
#pwd
#echo "</PRE>"
#echo "printenv"
#echo "<PRE>"
#printenv | sort
#echo "</PRE>"
#echo "set"
#echo "<PRE>"
#set | sort
#echo "</PRE>"

ARGS="`cat`"

echo "cat"
echo "<PRE>"
echo "$ARGS"
echo "</PRE>"

echo "begin gawk<p>"
gawk 'BEGIN {
    print "<H1>gawk</h1>";
    str="'$ARGS'";
    print str;
    argc=split(str,a,"\\&");
    print argc"<p>";
    for(t=1; t<=argc; t++)
    {
	split(a[t],a2,"=");
	print a2[1] " = "a2[2]"<P>";
	if (a2[1]=="email")
	{
	    email=a2[2];
	}
	else
	{
	    argv[t,"arg"]=a2[1];
	    argv[t,"val"]=a2[2];
	}
    }

    if (email=="")
    {
	print "Error: You must supply your EMail address.";
	exit(10);
    }
}' 2>&1 | cat
echo "end gawk<p>"

echo "</BODY></HTML>"

