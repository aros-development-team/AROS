BEGIN { chapter=0; section=0; subsection=0; fn="";
    file1="";
    file2="";
    file3="";
    fnhtml="";
    toc=0;
    printf ("") > "html.lab";
}
/\\chapter/ {
    if (match($0,/\\chapter{[^}]*}/))
    {
	if (section)
	    print "</OL>"
	if (chapter)
	    print "</OL>"
	chapter ++;
	checkfile();
	title=substr($0,RSTART+9,RLENGTH-10);
	prefix=chapter".";
	a[toc]="0:"prefix":"toc":"title;
	print "<OL>"
	date=getfiledate(fn);
	print "<H1><A HREF=\""fnhtml"#"toc"\">"prefix" "title"</A> ("date")</H1>";
	section=0; subsection=0;
	toc++;
    }
}
/\\section/ {
    if (match($0,/\\section{[^}]*}/))
    {
	if (section)
	    print "</OL>"
	section ++;
	checkfile();
	print "<OL>"
	title=substr($0,RSTART+9,RLENGTH-10);
	prefix=chapter"."section;
	a[toc]="1:"prefix":"toc":"title;
	print "<H2><A HREF=\""fnhtml"#"toc"\">"prefix" "title"</A></H2>";
	toc++;
	subsection=0;
    }
}
/\\subsection/ {
    if (match($0,/\\subsection{[^}]*}/))
    {
	subsection ++;
	checkfile();
	title=substr($0,RSTART+12,RLENGTH-13);
	prefix=chapter"."section"."subsection;
	a[toc]="2:"prefix":"toc":"title;
	print "<H3><A HREF=\""fnhtml"#"toc"\">"prefix" "title"</A></H3>";
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
	print "</OL>"
    if (chapter)
	print "</OL>"
    shiftfiles("adoc_index.html");
}

function checkfile() {
    if (fn!=FILENAME)
    {
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
	fninfo=file2;
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
