BEGIN {
    cols=3;
    if (mode=="pre_bylib")
    {
    }
    else if (mode=="post_bylib")
    {
	print "<DL>"
	first=1;
	dl=1;
    }
    if (mode=="pre_byname")
    {
    }
    else if (mode=="post_byname")
    {
	first=1;
	print "<DL>"
	dl=1;
	table=0;
    }
}
 {
    if (mode=="pre_bylib")
    {
	file=$1;
	gsub("..\/html\/","",file);
	first=toupper(substr($2,1,1));
	libs[$3]=libs[$3] first;
	print $3":"first":"$2":"file;
    }
    else if (mode=="post_bylib")
    {
	if ($1=="-")
	{
	    if (first)
	    {
		print "<A NAME=\"bylib\"></A><DT><FONT SIZE=7><B>A.2 Reference by library</B></FONT><DD><DL>"
		dl++;
		first=0;
		cnt=1;
	    }

	    print "<FONT SIZE=6>"

	    if ($2=="Utility functions")
		print "<A HREF=\"#util\">"
	    else
		print "<A HREF=\"#lib"$2"\">"

	    print "<B>A.2."cnt" "$2"</B></A> ("
	    cnt ++;

	    libini[$2]=$3;

	    initials=$3;
	    for (t=1; t<=length(initials); t++)
	    {
		c=substr(initials,t,1);
		print "<A HREF=\"#lib"$2 c"\"><FONT SIZE=6><B>"c"</B></FONT></A> "
	    }

	    print ")</FONT><BR>"

	    lib="";
	    char="";
	}
	else
	{
	    if (lib != $1)
	    {
		if (table)
		{
		    while (horiz && horiz < cols)
		    {
			print "<TD WIDTH=25%> </TD>"
			horiz++;
		    }
		    print "</TR></TABLE>"
		    table=0;
		}

		if (lib != "")
		{
		    print "</DL><P>"
		    dl--;
		}
		else
		{
		    print "<P>"
		    cnt=1;
		}

		lib = $1;

		print "<P><DT>"

		if (lib=="Utility functions")
		    print "<A NAME=\"util\"></A>"
		else
		    print "<A NAME=\"lib"lib"\"></A>"

		print "<FONT SIZE=6><B>A.2."cnt " " $1"</B></FONT><DD>"
		cnt ++;

		initials=libini[lib];
		for (t=1; t<=length(initials); t++)
		{
		    c=substr(initials,t,1);
		    print "<A HREF=\"#lib"lib c"\"><FONT SIZE=6><B>"c"</B></FONT></A> "
		}

		print "<P><DL>"
		dl++;
	    }
	    if (char != $2)
	    {
		char=$2;

		if (table)
		{
		    while (horiz && horiz < cols)
		    {
			print "<TD WIDTH=25%> </TD>"
			horiz++;
		    }
		    print "</TR></TABLE>"
		    table=0;
		}

		print "<DT>"
		print "<A NAME=\"lib"lib char"\"></A>"
		print "<FONT SIZE=5><B>"char"</B></FONT><DD>"

		print "<TABLE WIDTH=80%>"
		table=1;
		horiz=0;
	    }

	    if (horiz==0)
	    {
		print "<TR>"
	    }

	    print "<TD WIDTH=25%>"
	    print "<A HREF=\""$4"\">"$3"()</A> "
	    print "</TD>"
	    horiz++;

	    if (horiz==cols)
	    {
		print "</TR>"
		horiz=0;
	    }
	}
    }
    if (mode=="pre_byname")
    {
	first=substr($2,1,1);
	initials=initials first;
	file=$1;
	gsub("..\/html\/","",file);
	print toupper(first)":"$2":"file;
    }
    else if (mode=="post_byname")
    {
	if ($1=="-")
	{
	    print "<DT><A NAME=\"byname\"></A><FONT SIZE=7><B>A.3 Reference by name</B></FONT><DD>"

	    for (t=1; t<=length($2); t++)
	    {
		char=substr($2,t,1);
		print "<A HREF=\"#name"char"\"><FONT SIZE=6><B>"char"</B></FONT></A> "
	    }

	    print "<P><DL>"
	    dl++;

	    char="-";
	}
	else
	{
	    if (char!=$1)
	    {
		if (table)
		{
		    while (horiz && horiz < cols)
		    {
			print "<TD WIDTH=25%> </TD>"
			horiz++;
		    }
		    print "</TR></TABLE>"
		    table=0;
		}

		if (char != "-")
		{
		    print "</DL><P>"
		    dl--;
		}
		else
		    print "<P>"

		char = $1;

		print "<DT><A NAME=\"name"char"\"></A><FONT SIZE=5><B>"char"</B></FONT><DD>"
		print "<DL>"
		dl++;

		print "<TABLE WIDTH=80%>"
		table=1;
		horiz=0;
	    }

	    if (horiz==0)
	    {
		print "<TR>"
	    }

	    print "<TD WIDTH=25%>"
	    print "<A HREF=\"" $3 "\">" $2 "()</A>"
	    print "</TD>"

	    horiz++;

	    if (horiz==cols)
	    {
		print "</TR>"
		horiz=0;
	    }
	}
    }
}
END {
    if (mode=="pre_bylib")
    {
	alphabeth="ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for (lib in libs)
	{
	    out="";
	    for (t=1; t<=26; t++)
	    {
		if (index(libs[lib],substr(alphabeth,t,1)))
		    out=out substr(alphabeth,t,1);
	    }
	    print "-:"lib":"out;
	}
    }
    else if (mode=="post_bylib")
    {
	if (table)
	{
	    while (horiz && horiz < cols)
	    {
		print "<TD WIDTH=25%> </TD>"
		horiz++;
	    }
	    print "</TABLE>"
	}
	while (dl--)
	    print "</DL>"
    }
    if (mode=="pre_byname")
    {
	alphabeth="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	out="";
	for (t=1; t<=26; t++)
	{
	    if (index(initials,substr(alphabeth,t,1)))
		out=out substr(alphabeth,t,1);
	}
	print "-:" out;
    }
    else if (mode=="post_byname")
    {
	if (table)
	{
	    while (horiz && horiz < cols)
	    {
		print "<TD WIDTH=25%> </TD>"
		horiz++;
	    }
	    print "</TABLE>"
	}
	while (dl--)
	    print "</DL>"
    }
}
