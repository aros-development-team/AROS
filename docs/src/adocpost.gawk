BEGIN {
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
    }
}
 {
    if (mode=="pre_bylib")
    {
	if (!($3 in libs) )
	{
	    libs[$3]=1;
	    print "-:"$3;
	}
	file=$1;
	gsub("..\/html\/","",file);
	first=toupper(substr($2,1,1));
	print $3":"first":"$2":"file;
    }
    else if (mode=="post_bylib")
    {
	if ($1=="-")
	{
	    if (first)
	    {
		print "<A NAME=\"bylib\"></A><DT><FONT SIZE=7><B>Reference by library</B></FONT><DD><DL>"
		dl++;
		first=0;
	    }

	    if ($2=="Utility functions")
		print "<A HREF=\"#util\"><FONT SIZE=6><B>Utility functions</B></FONT></A> "
	    else
		print "<A HREF=\"#lib"$2"\"><FONT SIZE=6><B>"$2"</B></FONT></A> "

	    lib="";
	    char="";
	}
	else
	{
	    if (lib != $1)
	    {
		if (lib != "")
		{
		    print "</DL><P>"
		    dl--;
		}
		else
		    print "<P>"

		lib = $1;

		print "<DT>"

		if (lib=="Utility functions")
		    print "<A NAME=\"util\"></A><FONT SIZE=6><B>Utility functions</B></FONT>"
		else
		    print "<A NAME=\"lib"lib"\"></A><FONT SIZE=6><B>"$1"</B></FONT>"

		print "<DD><DL>"
		dl++;
	    }
	    if (char != $2)
	    {
		char=$2;
		print "<DT><FONT SIZE=5><B>"char"</B></FONT><DD>"
	    }

	    print "<A HREF=\""$4"\">"$3" ()</A> "
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
	    print "<DT><A NAME=\"byname\"></A><FONT SIZE=7><B>Reference by name</B></FONT><DD>"

	    for (t=1; t<length($2); t++)
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
	    }

	    print "<A HREF=\"" $3 "\">" $2 " ()</A>"
	}
    }
}
END {
    if (mode=="pre_bylib")
    {
    }
    else if (mode=="post_bylib")
    {
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
	while (dl--)
	    print "</DL>"
    }
}
