BEGIN {
    sp=0;
    skip=0;
    keeplf=0;
    lastwaslf=0;
    mode="text";

    special_items["LONG"] = "<A HREF=\"srcs/include/exec/types.h\">LONG</A>";
    special_items["ULONG"] = "<A HREF=\"srcs/include/exec/types.h\">ULONG</A>";
    special_items["IPTR"] = "<A HREF=\"srcs/include/exec/types.h\">IPTR</A>";
    special_items["WORD"] = "<A HREF=\"srcs/include/exec/types.h\">WORD</A>";
    special_items["UWORD"] = "<A HREF=\"srcs/include/exec/types.h\">UWORD</A>";
    special_items["BYTE"] = "<A HREF=\"srcs/include/exec/types.h\">BYTE</A>";
    special_items["UBYTE"] = "<A HREF=\"srcs/include/exec/types.h\">UBYTE</A>";
    special_items["QUAD"] = "<A HREF=\"srcs/include/exec/types.h\">QUAD</A>";
    special_items["UQUAD"] = "<A HREF=\"srcs/include/exec/types.h\">UQUAD</A>";
    special_items["APTR"] = "<A HREF=\"srcs/include/exec/types.h\">APTR</A>";
    special_items["STACKLONG"] = "<A HREF=\"srcs/include/exec/types.h\">STACKLONG</A>";
    special_items["STACKULONG"] = "<A HREF=\"srcs/include/exec/types.h\">STACKULONG</A>";
    special_items["STACKIPTR"] = "<A HREF=\"srcs/include/exec/types.h\">STACKIPTR</A>";
    special_items["STACKWORD"] = "<A HREF=\"srcs/include/exec/types.h\">STACKWORD</A>";
    special_items["STACKUWORD"] = "<A HREF=\"srcs/include/exec/types.h\">STACKUWORD</A>";
    special_items["STACKBYTE"] = "<A HREF=\"srcs/include/exec/types.h\">STACKBYTE</A>";
    special_items["STACKUBYTE"] = "<A HREF=\"srcs/include/exec/types.h\">STACKUBYTE</A>";
    special_items["STACKQUAD"] = "<A HREF=\"srcs/include/exec/types.h\">STACKQUAD</A>";
    special_items["STACKUQUAD"] = "<A HREF=\"srcs/include/exec/types.h\">STACKUQUAD</A>";
    special_items["STACKFLOAT"] = "<A HREF=\"srcs/include/exec/types.h\">STACKFLOAT</A>";

    special_items["TagItem"] = "<A HREF=\"srcs/include/utility/tagitem.h\">TagItem</A>";
    special_items["ti_Tag"] = "<A HREF=\"srcs/include/utility/tagitem.h\">ti_Tag</A>";
    special_items["ti_Data"] = "<A HREF=\"srcs/include/utility/tagitem.h\">ti_Data</A>";
    special_items["USER_TAG"] = "<A HREF=\"srcs/include/utility/tagitem.h\">USER_TAG</A>";

    special_items["CreateRastPort()"] = "<A HREF=\"srcs/graphics/createrastport.c\">CreateRastPort()</A>";
    special_items["FreeRastPort()"] = "<A HREF=\"srcs/graphics/freerastport.c\">FreeRastPort()</A>";
    special_items["InitRastPort()"] = "<A HREF=\"srcs/graphics/initrastport.c\">InitRastPort()</A>";
    special_items["DeinitRastPort()"] = "<A HREF=\"srcs/graphics/deinitrastport.c\">DeinitRastPort()</A>";
    special_items["CloneRastPort()"] = "<A HREF=\"srcs/graphics/clonerastport.c\">CloneRastPort()</A>";

    special_items["DoMethod()"] = "<A HREF=\"srcs/alib/boopsi.c\">DoMethod()</A>";
    special_items["DoMethodA()"] = "<A HREF=\"srcs/alib/boopsi.c\">DoMethodA()</A>";

    special_items["ReadArgs()"] = "<A HREF=\"srcs/dos/readargs.c\">ReadArgs()</A>";
    special_items["VPrintf()"] = "<A HREF=\"srcs/dos/vprintf.c\">VPrintf()</A>";
    special_items["VFPrintf()"] = "<A HREF=\"srcs/dos/vfprintf.c\">VFPrintf()</A>";

    special_items["SMult64()"] = "<A HREF=\"srcs/utility/smult64.c\">SMult64()</A>";

    cmd="date \"+%d %b %Y\""
    cmd | getline today;
    close (cmd);

    if (ARGC==2)
    {
	fninfo="gen/"ARGV[1];
	gsub(/.src$/,".info",fninfo);
    }
    else
	fninfo="";

    toc=0;
    thischapter="";

    if (fninfo!="")
    {
	while ((getline < fninfo) > 0)
	{
	    if ($1=="prev")
		prev_doc=$2;
	    else if ($1=="next")
		next_doc=$2;
	    else if ($1=="toc")
	    {
		a_toc[toc]=substr($0,5);
		n=split(a_toc[toc],a,":");

		if (thischapter=="" && match(a[2],/\.$/))
		    thischapter=a[4];

		toc ++;
	    }
	}

	close (fninfo);
    }

    if (prev_doc!="")
	prev_doc="<A HREF=\""prev_doc"\">previous</A>";
    if (next_doc!="")
	next_doc="<A HREF=\""next_doc"\">next</A>";

    if (fninfo!="")
    {
	file="page_header.html";

	while ((getline < file) > 0)
	{
	    gsub(/\\thischapter/,thischapter);
	    print
	}

	close (file);
    }

    if (toc > 0)
    {
	print "<CENTER><FONT SIZE=7><B>Table of Contents</B></FONT></CENTER>"
	print "<OL>"
	level=0;

	for (t=0; t<toc; t++)
	{
	    n=split(a_toc[t],a,":");

	    while (level < a[1])
	    {
		print "<OL>"
		level ++;
	    }
	    while (level > a[1])
	    {
		print "</OL>"
		level --;
	    }

	    prefix=a[2];
	    no=a[3];
	    title=a[4];

	    print "<H"level+1"><A HREF=\"#"no"\">"prefix" "title"</A></H"level+1">"
	}

	while (level > 0)
	{
	    print "</OL>"
	    level --;
	}

	print "</OL>"
	print "<CENTER><P><HR WIDTH=\"100%\"></P></CENTER>"

    }

    toc=0;
    while ((token=yylex()) != "")
    {
#print "token="token" yytext=\""yytext"\"" >> "/dev/stderr";

	if (token=="cmd")
	{
	    if (yytext=="chapter")
	    {
		getarg();
		split(a_toc[toc],a,":");
		yytext="\n\n<H1><A NAME=\""toc"\">"a[2]" "yytext"</A></H1>"
		skippar=1;
		toc++;
	    }
	    else if (yytext=="section")
	    {
		getarg();
		split(a_toc[toc],a,":");
		yytext="\n\n<H2><A NAME=\""toc"\">"a[2]" "yytext"</A></H2>"
		skippar=1;
		toc++;
	    }
	    else if (yytext=="subsection")
	    {
		getarg();
		split(a_toc[toc],a,":");
		yytext="\n\n<H3><A NAME=\""toc"\">"a[2]" "yytext"</A></H3>"
		skippar=1;
		toc++;
	    }
	    else if (yytext=="par")
	    {
		if (skippar)
		{
		    skippar = 0;
		    yytext="";
		}
		else
		{
		    if (!keeplf)
		    {
			yytext="";
			showpar=1;
		    }
		    else
		    {
			yytext="\n";
			lastwaslf=1;
			skippar=0;
		    }
		}
	    }
	    else if (yytext=="label")
	    {
		getarg();
		label=yytext;

		yytext="<A NAME=\""label"\">";
	    }
	    else if (yytext=="lref")
	    {
		getarg();
		text=yytext;
		getarg();
		label=yytext;

		found=0;

		while ((getline < "html.lab") > 0)
		{
		    if ($1==label)
		    {
			file=$2;
			found=1;
			break;
		    }
		}

		close ("html.lab");

		if (!found)
		    yytext="!!!! Unknown label " label " !!!!";
		else
		    yytext="<A HREF=\""file"#"label"\">"text"</A>";
	    }
	    else if (yytext=="begin")
	    {
		stack[sp] = mode;
		sp ++;
		getarg();
		mode=yytext;

		if (mode=="description")
		{
		    yytext="\n\n<DL>"
		}
		else if (mode=="itemize")
		{
		    yytext="\n\n<UL>"
		}
		else if (mode=="enumeration")
		{
		    yytext="\n\n<OL TYPE=1>"
		}
		else if (mode=="emph")
		{
		    yytext="\n\n<P>\n<UL>\n<EM>";
		}
		else if (mode=="example")
		{
		    printf ("<UL><PRE>");

		    while ((getline line) > 0)
		    {
			if (match(line,/\\end{example}/))
			    break;
			else if (match(line,/\\link{.*}{.*}/))
			{
			    printf ("%s", substr(line,1,RSTART-1));
			    yyrest=substr(line,RSTART+5,RLENGTH-5);
			    rest=substr(line,RSTART+RLENGTH);

			    match(yyrest,/^{[^}]*}/);
			    title=substr(yyrest,RSTART+1,RLENGTH-2);
			    link=substr(yyrest,RSTART+RLENGTH+1);
			    link=substr(link,1,length(link)-1);

			    printf ("<A HREF=\""link"\">"title"</A>%s\n", rest);
			}
			else
			{
			    gsub(/&/,"\\&amp;",line);
			    gsub(/</,"\\&lt;",line);
			    gsub(/>/,"\\&gt;",line);
			    gsub(/"/,"\\&quot;",line);

			    print line;
			}
		    }

		    print "</PRE></UL>";
		    yytext="";
		    yyrest="";
		    sp --;
		    mode=stack[sp];
		}
		else
		    yytext="!!!! Unknown mode " yytext " !!!!\n";
	    }
	    else if (yytext=="end")
	    {
		getarg();
		endmode=yytext;
		if (endmode != mode)
		{
		    print "ERROR: \\end{"endmode"} doesn't match \\begin{"mode"}" >> "/dev/stderr"
		    exit 10;
		}

		if (mode=="description")
		{
		    yytext="\n</DL>"
		}
		else if (mode=="itemize")
		{
		    yytext="\n</UL>"
		}
		else if (mode=="enumeration")
		{
		    yytext="\n\n</OL>"
		}
		else if (mode=="emph")
		{
		    yytext="</EM>\n</UL>\n";
		}
		else if (mode=="example")
		{
		    yytext="</PRE></UL>";
		    keeplf=0;
		}
		else
		    yytext="";

		sp --;
		mode=stack[sp];
	    }
	    else if (yytext=="item")
	    {
		if (mode=="description")
		{
		    getarg();

		    gsub(/&/,"\\&amp;",yytext);
		    gsub(/</,"\\&lt;",yytext);
		    gsub(/>/,"\\&gt;",yytext);
		    gsub(/"/,"\\&quot;",yytext);

		    yytext="<P>\n<DT><I>"yytext"</I><DD>"
		}
		else if (mode=="itemize")
		{
		    yytext="\n<LI>"
		}
		else if (mode=="enumeration")
		{
		    yytext="\n<LI>"
		}
		else
		    yytext="";

		showpar=0;
	    }
	    else if (yytext=="exec")
	    {
		getarg();
		gsub(/\$[(]TOP[)]/,TOP,yytext);
		cmd=yytext " | gawk -f src2html.gawk --assign TOP=\""TOP"\"";

		while ((cmd | getline) > 0)
		    print;

		close (cmd);

		yytext="";
	    }
	    else if (yytext=="execverb")
	    {
		getarg();
		gsub(/\$[(]TOP[)]/,TOP,yytext);
		cmd=yytext;

		while ((cmd | getline) > 0)
		    print;

		close (cmd);

		yytext="";
	    }
	    else if (yytext=="largepic")
	    {
		getarg();
		title=yytext;
		getarg();
		file=yytext;

		if (showpar)
		    printf ("\n<P>");

		yytext="<A HREF=\""file"\">"title"</A>";
	    }
	    else if (yytext=="bold")
	    {
		getarg();

		yytext="<B>"yytext"</B>";
	    }
	    else if (yytext=="italics")
	    {
		getarg();

		yytext="<I>"yytext"</I>";
	    }
	    else if (yytext=="shell")
	    {
		getarg();

		yytext="<TT><B>"yytext"</B></TT>";
	    }
	    else if (yytext=="email")
	    {
		getarg();

		yytext="<A HREF=\"mailto:"yytext"\">"yytext"</A>";
	    }
	    else if (yytext=="filename")
	    {
		getarg();
		file=yytext;

		if (!match(file,/\$/))
		{
		    if (match(file,/^[a-zA-Z]+/))
		    {
			prefix=substr(file,RSTART,RLENGTH);

			if (prefix=="AROS")
			{
			    file="<A HREF=\"srcs/"substr(file,6)"\">"file"</A>"
			}
		    }
		}

		yytext="<I><TT>" file "</TT></I>";
	    }
	    else if (yytext=="link")
	    {
		getarg();
		title=yytext;
		getarg();
		link=yytext;

		yytext="<A HREF=\""link"\">"title"</A>";
	    }
	    else
	    {
		yytext="\n\n!!!!!!!!\n Unknown cmd: \"" yytext "\"\n!!!!!!!\n\n"
	    }
	}
	else if (token=="code")
	{
	    if (yytext in special_items)
		yytext=special_items[yytext];

	    yytext="<TT>" yytext "</TT>";
	}
	else if (token=="text")
	{
	    if (showpar && !skippar)
	    {
		#if (mode=="text")
		#{
		    gsub(/[ \t\n]+/," ",yytext);

		    if (yytext != " ")
		    {
			showpar=0;
			printf ("\n<P>");
		    }
		#}
	    }
	    if (mode=="example")
	    {
		if (lastwaslf)
		{
		    lastwaslf=0;
		    #gsub(/^[ \t]+/,"",yytext);
		}
	    }
	}

	printf ("%s", yytext);
    } # while

    print ""

    if (fninfo!="")
    {
	file="page_footer.html";

	while ((getline < file) > 0)
	{
	    gsub(/\\prev/,prev_doc);
	    gsub(/\\next/,next_doc);
	    gsub(/\\today/,today);
	    print
	}
    }
}

function yylex() {
    if (yyrest=="")
    {
	if (getline yyrest <= 0)
	    return ""; #EOF

	if (yyrest!="")
	    yyrest=yyrest " ";

	if (keeplf)
	{
	    yytext="par";
	    return "cmd";
	}
    }

    if (match(yyrest,/^\\[a-z]+/))
    {
	yytext=substr(yyrest,RSTART+1,RLENGTH-1);
	yyrest=substr(yyrest,RSTART+RLENGTH);

	return "cmd";
    }
    else if (match(yyrest,/^\\\\/))
    {
	yytext="\\";
	yyrest=substr(yyrest,3);
	return "text";
    }
    else if (match(yyrest,/^\|[^\|]+\|/))
    {
	yytext=substr(yyrest,RSTART+1,RLENGTH-2);
	yyrest=substr(yyrest,RSTART+RLENGTH);

	return "code";
    }
    else if (match(yyrest,/^{[^}]*}/))
    {
	yytext=substr(yyrest,RSTART+1,RLENGTH-2);
	yyrest=substr(yyrest,RSTART+RLENGTH);

	return "arg";
    }
    else if (match(yyrest,/^[^\\\|{]+/))
    {
	yytext=substr(yyrest,RSTART,RLENGTH);
	yyrest=substr(yyrest,RSTART+RLENGTH);

	gsub(/&/,"\\&amp;",yytext);
	gsub(/</,"\\&lt;",yytext);
	gsub(/>/,"\\&gt;",yytext);
	gsub(/"/,"\\&quot;",yytext);

	return "text";
    }
    else
    {
	if (yyrest=="")
	{
	    if (!keeplf)
	    {
		do
		{
		    ret=getline yyrest;
		}
		while (yyrest=="" && ret>0);

		if (ret<=0)
		    return "";

		yyrest=yyrest " ";
	    }

	    yytext="par";

	    return "cmd";
	}

	yytext=substr(yyrest,1,1);
	yyrest=substr(yyrest,2);

	# multiline
	if (yytext=="|")
	{
	}
	else if (yytext=="{")
	{
	}
	else
	{
	    gsub(/&/,"\\&amp;",yytext);
	    gsub(/</,"\\&lt;",yytext);
	    gsub(/>/,"\\&gt;",yytext);
	    gsub(/"/,"\\&quot;",yytext);
	}

	return "text";
    }
}

function getarg(token) {
    while ((token=yylex()) != "")
    {
	if (token=="arg")
	{
#print "token=arg yytext=\""yytext"\"" >> "/dev/stderr";
	    return;
	}
    }

    print "Missing argument in "FILENAME":"FN"\n" >>"/dev/stderr";
    exit 10;
}
