BEGIN {
    for(t=1; t<ARGC; t++)
    {
	file=ARGV[t];
	printf ("%3d%% %-60s", t*100/ARGC, basename(file)) >> "/dev/stderr";
	fflush("/dev/stderr");

	out="";
	mode="head";

	while ((getline < file) > 0)
	{
	    if (mode=="head")
	    {
		if (match($0,/^.\*\*\*\*\*+$/))
		{
		    out="../html/autodocs/" basename(file) ".html";
		    print "<HTML><HEAD>\n<TITLE>AROS - The Amiga Replacement OS - AutoDocs</TITLE>\n</HEAD>\n<BODY>\n" > out;
		    print "<CENTER><P>(C) 1996 AROS - The Amiga Replacement OS</P></CENTER>\n<P><HR></P>\n\n" >> out;
		    mode="field";
		    lastfield="";
		}
	    }
	    else if (mode=="field")
	    {
		newfield=$1;

		if (newfield!="")
		{
		    if (newfield=="#" || newfield=="/*")
			newfield=$2;
		}

		if (newfield=="NAME" || newfield=="SYNOPSIS" || newfield=="LOCATION" ||
		    newfield=="FUNCTION" || newfield=="INPUTS" || newfield=="RESULT" ||
		    newfield=="NOTES" || newfield=="EXAMPLE" || newfield=="BUGS" ||
		    newfield=="SEE" || newfield=="INTERNALS" || newfield=="HISTORY")
		{
		    if (lastfield=="EXAMPLE")
			print "</PRE>" >> out;
		    else if (field=="INPUTS")
			print "</DL>\n" >> out;

		    if (lastfield!="")
			print "</DL>\n" >> out;

		    print "<DL>\n<DT>"newfield"\n<DD>\n" >> out;
		    mode="field";
		    lastfield=newfield;
		    first=1;
		    field=newfield;

		    if (field=="EXAMPLE")
			print "<PRE>" >> out;
		    else if (field=="INPUTS")
			print "<DL>\n" >> out;
		}
		else if (match($0,/^.\*\*\*\*\*+\/?$/))
		{
		    mode="footer";
		}
		else
		{
		    if ($1=="#")
			line=substr($0,2);
		    else
			line=$0;

		    if (field=="NAME")
		    {
			if (first)
			{
			    first=0;
			    prefix="";
			}
			else
			{
			    prefix="<P>";
			}

			if (match(line,/AROS_LH/))
			{
			    gsub(/,[ \t]/,",",line);
			    split(line,a,",");
			    print prefix a[2] >> out;
			    print a[2];
			}
			else if (match(line,"#include"))
			{
			    match (line,/<.*>/);
			    hfile=substr(line,RSTART+1,RLENGTH-2);
			    link="../../../include/" hfile;

			    err=getline < link;

			    if (err > 0)
			    {
				close (link);

				line="<A HREF=\""link"\">"hfile"</A>";
			    }
			    else
				line=hfile;

			    print prefix "#include &lt;"line"&gt;" >> out;
			}
		    }
		    else if (field=="SYNOPSIS")
		    {
			if (first)
			    first=0;
			else
			    line="<P>" line;

			gsub(/AROS_LHA[(]/,"",line);
			gsub(/[)],/,"",line);
			gsub(/,[ \t]/,",",line);

			split(line,a,",");
			print "<TT>"a[1]" "a[2]"</TT>" >> out;
		    }
		    else if (field=="LOCATION")
		    {
			if (match(line,/,/))
			{
			    gsub(/[)]/,"",line);
			    gsub(/,[ \t]/,",",line);

			    split(line,a,",");
			    print "In " a[2] " at offset " a[3] >> out;
			}
		    }
		    else if (field=="INPUTS")
		    {
			if (match(line,/[ \t]*([a-zA-Z_]+,[ \t]*)*[a-zA-Z_]+[ \t]*-/))
			{
			    print "<DT>"substr(line,RSTART,RLENGTH-1) >> out;
			    print "<DD>"substr(line,RSTART+RLENGTH) >> out;
			}
			else
			    print line >> out;

			if (line=="" && !first)
			    print "<P>\n" >> out;
			else
			    print line >> out;

			first=0;
		    }
		    else if (field=="FUNCTION" || field=="RESULT" ||
			field=="NOTES" || field=="BUGS" || field=="INTERNALS")
		    {
			if (line=="" && !first)
			    print "<P>\n" >> out;
			else
			    print line >> out;

			first=0;
		    }
		    else
			print line >> out;
		}
	    }
	    else if (mode=="footer")
	    {
	    }
	}

	if (out!="")
	    print "</BODY></HTML>" >> out;

	close (out);
	close (file);

	printf ("\r", t*100/ARGC) >> "/dev/stderr";
    }

    print "\n" >> "/dev/stderr";
}

function basename(file) {
    return gensub(/.*\/([a-z]+)(\.[a-z]+)?$/,"\\1",1,file);
}
