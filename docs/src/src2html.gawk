BEGIN {
    toc=0;
    sp=0;
    skip=0;
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

    fninfo=ARGV[1];
    gsub(/.src$/,".info",fninfo);

    if (fninfo!="")
    {
	while ((getline < fninfo) > 0)
	{
	    if ($1=="prev")
		prev_doc=$2;
	    else if ($1=="next")
		next_doc=$2;
	}
    }

    if (prev_doc!="")
	prev_doc="<A HREF=\""prev_doc"\">previous</A>";
    if (next_doc!="")
	next_doc="<A HREF=\""next_doc"\">next</A>";

    if (fninfo!="")
    {
	file="page_header.html";

	while ((getline < file) > 0)
	    print
    }

    while ((token=yylex()) != "")
    {
	if (token=="cmd")
	{
	    if (yytext=="chapter")
	    {
		getarg();
		yytext="\n\n<H1><A NAME=\""toc"\">"yytext"</A></H1>"
		skippar=1;
		toc++;
	    }
	    else if (yytext=="section")
	    {
		getarg();
		yytext="\n\n<H2><A NAME=\""toc"\">"yytext"</A></H2>"
		skippar=1;
		toc++;
	    }
	    else if (yytext=="subsection")
	    {
		getarg();
		yytext="\n\n<H3><A NAME=\""toc"\">"yytext"</A></H3>"
		skippar=1;
		toc++;
	    }
	    else if (yytext=="par")
	    {
		if (skippar)
		    skippar = 0;

		yytext="";
		showpar=1;
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
		    yytext="\n\n<DL>"
		}
		else if (mode=="emph")
		{
		    yytext="\n\n<P>\n<UL>\n<EM>";
		}
		else if (mode=="example")
		{
		    yytext="\n\n<PRE>";
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
		else if (mode=="emph")
		{
		    yytext="</EM>\n</UL>\n";
		}
		else if (mode=="example")
		{
		    yytext="</PRE>";
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
		    yytext="\n<LI><B>"yytext"</B>"
		}
		else if (mode=="itemize")
		{
		    yytext="\n<LI>"
		}
		else
		    yytext="";
	    }
	    else if (yytext=="exec")
	    {
		getarg();
		cmd=yytext " | gawk -f src2html.gawk";

		#print cmd;

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

		yytext="<A HREF=\""file"\">"title"</A>";
	    }
	    else if (yytext=="bold")
	    {
		getarg();

		yytext="<B>"yytext"</B>";
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
		if (mode=="text")
		{
		    gsub(/[ \t\n]+/," ",yytext);

		    if (yytext != " ")
		    {
			showpar=0;
			printf ("\n\n<P>");
		    }
		}
		else if (mode=="example")
		{
		    yytext=yytext"\n    ";
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
    }

    if (match(yyrest,/^\\[a-z]+/))
    {
	yytext=substr(yyrest,RSTART+1,RLENGTH-1);
	yyrest=substr(yyrest,RSTART+RLENGTH);

	return "cmd";
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
	gsub(/ +/," ",yytext);

	return "text";
    }
    else
    {
	if (yyrest=="")
	{
	    do
	    {
		ret=getline yyrest;
	    }
	    while (yyrest=="" && ret>0);

	    if (ret<=0)
		return "";

	    yytext="par";
	    yyrest=yyrest " ";

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
	    return;
    }

    print "Missing argument in "FILENAME":"FN"\n" >>"/dev/stderr";
    exit 10;
}
