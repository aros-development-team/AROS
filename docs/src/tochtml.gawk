BEGIN { chapter=0; section=0; subsection=0; fn="";
    file1="";
    file2="";
    file3="";
    fnhtml="";
    toc=0;
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
	print "<OL>"
	print "<H1><A HREF=\""fnhtml"#"toc"\">"chapter". "substr($0,RSTART+9,RLENGTH-10)"</A></H1>";
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
	print "<H2><A HREF=\""fnhtml"#"toc"\">"chapter"."section" "substr($0,RSTART+9,RLENGTH-10)"</A></H2>";
	toc++;
	subsection=0;
    }
}
/\\subsection/ {
    if (match($0,/\\subsection{[^}]*}/))
    {
	subsection ++;
	checkfile();
	print "<H3><A HREF=\""fnhtml"#"toc"."subsection"\">"chapter"."section"."subsection" "substr($0,RSTART+12,RLENGTH-13)"</A></H3>";
	toc++;
    }
}
END {
    if (section)
	print "</OL>"
    if (chapter)
	print "</OL>"
    shiftfiles("");
}

function checkfile() {
    if (fn!=FILENAME)
    {
	fn=FILENAME;
	fnhtml=fn;
	fninfo=fn;
	sub(/.src$/,".html",fnhtml);
	toc=0;

	shiftfiles(fnhtml);
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

	close (fninfo);
    }
}
