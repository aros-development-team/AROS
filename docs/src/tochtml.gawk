BEGIN { chapter=0; section=0; subsection=0; fn="";
    file1="";
    file2="";
    file3="";
    fnhtml="";
    toc=0;
    newfile=0;
    printf ("") > "html.lab";
}
/\\chapter/ {
    if (match($0,/\\chapter{[^}]*}/))
    {
	if (subsection)
	    print "</UL>"
	if (section)
	    print "</UL>"
	if (chapter)
	    print "</UL>"
	chapter ++;
	checkfile();
	title=substr($0,RSTART+9,RLENGTH-10);
	prefix=chapter".";
	a[toc]="0:"prefix":"toc":"title;
	print "<UL>"
	if (newfile)
	{
	    newfile=0;
	    date=" ("getfiledate(fn)")";
	}
	else
	    date="";
	print "<LI><FONT SIZE=\"+3\"><A HREF=\""fnhtml"#"toc"\">"prefix" "title"</A>"date"</FONT>";
	section=0; subsection=0;
	toc++;
    }
}
/\\section/ {
    if (match($0,/\\section{[^}]*}/))
    {
	if (subsection)
	    print "</UL>"
	if (section)
	    print "</UL>"
	section ++;
	checkfile();
	print "<UL>"
	title=substr($0,RSTART+9,RLENGTH-10);
	prefix=chapter"."section;
	a[toc]="1:"prefix":"toc":"title;
	if (newfile)
	{
	    newfile=0;
	    date=" ("getfiledate(fn)")";
	}
	else
	    date="";
	print "<LI><FONT SIZE=\"+2\"><A HREF=\""fnhtml"#"toc"\">"prefix" "title"</A>"date"</FONT>";
	toc++;
	subsection=0;
    }
}
/\\subsection/ {
    if (match($0,/\\subsection{[^}]*}/))
    {
	if (!subsection)
	    print "<UL>"
	subsection ++;
	checkfile();
	title=substr($0,RSTART+12,RLENGTH-13);
	prefix=chapter"."section"."subsection;
	a[toc]="2:"prefix":"toc":"title;
	if (newfile)
	{
	    newfile=0;
	    date=" ("getfiledate(fn)")";
	}
	else
	    date="";
	print "<LI><FONT SIZE=\"+1\"><A HREF=\""fnhtml"#"toc"\">"prefix" "title"</A>"date"</FONT>";
	toc++;
    }
}
/\\label/ {
    if (match($0,/\\label{[^}]*}/))
    {
	label=substr($0,RSTART+7,RLENGTH-8);
	f=FILENAME;
	sub(/.src$/,".html",f);
	print label " " f >> "html.lab";
    }
}
END {
    if (section)
	print "</UL>"
    if (chapter)
	print "</UL>"
    shiftfiles("adoc_index.html");
}

function checkfile() {
    if (fn!=FILENAME)
    {
	newfile=1;
	fn=FILENAME;
	fnhtml=fn;
	fninfo=fn;
	sub(/.src$/,".html",fnhtml);

	shiftfiles(fnhtml);

	toc=0;
    }
}

function shiftfiles(fn      ,fninfo) {
    file1=file2;
    file2=file3;
    file3=fn;
#printf "shift " fn "\n" >> "/dev/stderr";

    if (file2!="")
    {
	fninfo="gen/"file2;
	sub(/.html$/,".info",fninfo);

	print "prev " file1 > fninfo;
	print "next " file3 >> fninfo;

	for (t=0; t<toc; t++)
	    print "toc " a[t] >> fninfo;

	close (fninfo);
    }
}

function getfiledate(fn     ,date) {
    cmd="getfiledate "fn;

    cmd | getline date;
    close (cmd);

    return date;
}
