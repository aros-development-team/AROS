BEGIN {
    stderr="/dev/stderr";

    special_item["NEWLIST()"]="<A HREF=\"../srcs/include/exec/lists.h\">NEWLIST()</A>";

    for(t=1; t<ARGC; t++)
    {
	file=ARGV[t];
	bn=basename(file);
#	 printf ("%3d%% %-60s\r", t*100/ARGC, bn) >> stderr;
#	 if (substr(bn,1,1)=="6")
#	     print "\n"file >> stderr;
#	 fflush(stderr);

	LIBS["clib"]="ANSI C linklib";
	LIBS["alib"]="amiga.lib";
	LIBS["devs"]="Devices";
	LIBS["aros"]="AROS";
	LIBS["intuition"]="Intuition";

	out="";
	mode="head";
	fnr=0;

	while ((getline < file) > 0)
	{
	    fnr ++;

	    if (mode=="head")
	    {
		if (match($0,/^.\*\*\*\*\*+$/))
		{
		    fname="";
		    lib=file;
		    gsub(/\/[^/]+$/,"",lib);
		    gsub(/^.*\//,"",lib);
		    location=0;

		    out="../html/autodocs/" bn ".html";
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
		    {
			if (example_is=="here" || example_is=="example")
			    printf ("</PRE>") >> out;
		    }
		    else if (field=="INPUTS" || field=="HISTORY")
			print "</DL>\n" >> out;

		    if (lastfield!="")
			print "</DL>\n" >> out;

		    mode="field";
		    lastfield=newfield;
		    first=1;
		    field=newfield;

		    if (newfield=="SEE")
			print "<DL>\n<DT>SEE ALSO\n<DD>" >> out;
		    else
			print "<DL>\n<DT>"newfield"\n<DD>" >> out;

		    if (field=="EXAMPLE")
		    {
			example_is="";
		    }
		    else if (field=="INPUTS")
			print "<DL COMPACT>\n" >> out;
		    else if (field=="HISTORY")
			print "<DL COMPACT>\n" >> out;
		}
		else if (match($0,/^.\*\*\*\*\*+\/?$/))
		{
		    if (fname!="")
		    {
			if (!location)
			{
			    if (!(lib in LIBS) )
			    {
				print "Unknown lib: "lib" in file "file >> stderr;
			    }
			    else
				print out":"fname":"LIBS[lib];
			}
			else
			    print out":"fname":"lib;
		    }
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
			if (match(line,/[(]/))
			{
			    gsub(/,[ \t]/,",",line);
			    if (split(line,a,",")==3)
			    {
				gsub(/AROS_(L|UF)H.*[(]/,"",a[1]);
				gsub(/[ \t]*[(][ \t]*/,"",a[1]);
				print prefix a[1] " " a[2] "()<BR>" >> out;
				fname=a[2];
			    }
			    else
			    {
				fname=line;
				gsub(/[ \t]*[(][ \t]*$/,"",fname);
				type=fname;
				gsub(/^.*[ \t]+/,"",fname);
				gsub(/[ \t]+[^ \t]+$/,"",type);
				print prefix type " " fname "()<BR>" >> out;
			    }
			}
			else if (match(line,"#include"))
			{
			    match (line,/<.*>/);
			    hfile=substr(line,RSTART+1,RLENGTH-2);
			    link="srcs/include/" hfile;

			    if (exists("../html/" link))
				line="<A HREF=\"../"link"\">"hfile"</A>";
			    else
				line=hfile;

			    print prefix "#include &lt;"line"&gt;<BR>" >> out;
			}
		    }
		    else if (field=="SYNOPSIS")
		    {
			gsub(/AROS_(L|UF)HA[(]/,"",line);
			gsub(/[)][ \t]*,?/,"",line);
			gsub(/,[ \t]/,",",line);

			split(line,a,",");
			print "<TT>"a[1]" "a[2]"</TT><BR>" >> out;
		    }
		    else if (field=="LOCATION")
		    {
			if (match(line,/,/))
			{
			    gsub(/[)]/,"",line);
			    gsub(/,[ \t]/,",",line);

			    split(line,a,",");
			    print "In " a[2] " at offset " a[3] >> out;
			    lib=a[2];
			    location=1;
			}
		    }
		    else if (field=="INPUTS")
		    {
			if (match(line,/[ \t]*([a-zA-Z_]+,[ \t]*)*[a-zA-Z_]+[ \t]*-/))
			{
			    print "<DT>"substr(line,RSTART,RLENGTH-1) >> out;

			    line=substr(line,RSTART+RLENGTH);
			    gsub(/&/,"\\&amp;",line);
			    gsub(/</,"\\&lt;",line);
			    gsub(/>/,"\\&gt;",line);
			    gsub(/"/,"\\&quot;",line);

			    print "<DD>"line >> out;
			}
			else
			{
			    gsub(/&/,"\\&amp;",line);
			    gsub(/</,"\\&lt;",line);
			    gsub(/>/,"\\&gt;",line);
			    gsub(/"/,"\\&quot;",line);

			    print line >> out;
			}

			#if (line=="" && !first)
			#    print "<P>\n" >> out;
			#else
			#    print line >> out;
			#
			#first=0;
		    }
		    else if (field=="FUNCTION" || field=="RESULT" ||
			field=="NOTES" || field=="BUGS" || field=="INTERNALS")
		    {
			if (line=="" && !first)
			    print "<P>\n" >> out;
			else
			{
			    gsub(/&/,"\\&amp;",line);
			    gsub(/</,"\\&lt;",line);
			    gsub(/>/,"\\&gt;",line);
			    gsub(/"/,"\\&quot;",line);

			    print line >> out;
			}

			first=0;
		    }
		    else if (field=="EXAMPLE")
		    {
			if (first)
			{
			    if (match (line,/#[ \t]*ifdef[ \t]+EXAMPLE/))
			    {
				printf ("<PRE>")>>out;
				example_is = "example";
				ifdeflevel = 0;
				skip = 1;
			    }
			    else if (match (line,/See[ \t]+[^ \t]+[ \t]*[(][)]/))
			    {
				example_is = line;
				sub(/^.*See[ \t]+/,"",example_is);
				sub(/[ \t]*[(][)].*$/,"",example_is);
				fn = tolower(example_is) ".html";
				print "See <A HREF=\""fn"\">"example_is"()</A>">>out;
				skip=1;
			    }
			    else if (match (line,/See[ \t]below/))
			    {
				example_is="below";
				close (file);
				state = "skip";
				ifdeflevel = 0;

				printf ("<PRE>")>>out;
				while ((getline line < file) > 0)
				{
				    if (state=="skip")
				    {
					if (match (line,/^#[ \t]*ifdef[ \t]+TEST/))
					    state="found";
				    }
				    else if (state=="found")
				    {
					if (match (line,/^#[ \t]*ifdef/))
					    ifdeflevel ++;
					else if (match (line,/^#[ \t]*endif/))
					{
					    if (!ifdeflevel)
					    {
						state="skip";
						continue;
					    }
					    else
						ifdeflevel --;
					}

					print line >> out;
				    }
				}
				printf ("</PRE>")>>out;

				close (file);

				for (pos=0; pos<fnr; pos++)
				    getline line < file;

				skip=1;
			    }
			    else
				example_is="here";

			    first = 0;
			}

			if (example_is=="example" && !skip)
			{
			    if (match (line,/^#[ \t]*ifdef/))
				ifdeflevel ++;
			    else if (match (line,/^#[ \t]*endif/))
			    {
				if (!ifdeflevel)
				    skip=1;

				ifdeflevel --;
			    }
			}

			if (!skip)
			{
			    gsub(/^(        |\t)/,"",line);
			    gsub(/&/,"\\&amp;",line);
			    gsub(/</,"\\&lt;",line);
			    gsub(/>/,"\\&gt;",line);
			    gsub(/"/,"\\&quot;",line);

			    print line >> out;
			}
			else
			    skip --;
		    }
		    else if (field=="SEE")
		    {
			if (line!="")
			{
			    rest=line;
			    line="";

			    while (rest!="")
			    {
				if (match(rest,/^[a-zA-Z_][a-zA-Z0-9]*[(][)]/))
				{
				    link=substr(rest,RSTART,RLENGTH-2);
				    rest=substr(rest,RSTART+RLENGTH);

				    lfile="../html/autodocs/" tolower(link) ".html";

				    if (link in special_item)
					line = line special_item[link];
				    else if (link"()" in special_item)
					line = line special_item[link"()"];
				    else if (exists(lfile))
					line=line "<A HREF=\""tolower(link)".html\">"link"()</A>";
				    else
					line=line " " link "()";
				}
				else
				{
				    if (match(rest,/^[^a-zA-Z_]+/))
					len=RLENGTH;
				    else
					len=1;

				    post=substr(rest,1,len);

				    gsub(/&/,"\\&amp;",post);
				    gsub(/</,"\\&lt;",post);
				    gsub(/>/,"\\&gt;",post);
				    gsub(/"/,"\\&quot;",post);

				    line=line post;
				    rest=substr(rest,len+1);
				}
			    }

			    print line >> out;
			}
		    }
		    else if (field=="HISTORY")
		    {
			if (match(line,/^[ \t]+[0-3][0-9][-.][01][0-9][-.][0-9][0-9][ \t]+/))
			{
			    date=substr(line,RSTART,RLENGTH);
			    line=substr(line,RSTART+RLENGTH);
			    match(line,/^[^ \t]+/);
			    name=substr(line,RSTART,RLENGTH);
			    line=substr(line,RSTART+RLENGTH);
			    gsub(/^[ \t]+/,"",date);
			    gsub(/[ \t]+$/,"",date);

			    print "<DT>"date " " name"<DD>" >> out;
			}

			gsub(/&/,"\\&amp;",line);
			gsub(/</,"\\&lt;",line);
			gsub(/>/,"\\&gt;",line);
			gsub(/"/,"\\&quot;",line);

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
    }

#    printf ("\n") >> stderr;
}

function basename(file) {
    return gensub(/.*\/([a-zA-Z0-9_]+)(\.[a-zA-Z0-9_]+)?$/,"\\1",1,file) "";
}

function exists(file        ,err) {
    err=getline < file;

    if (err > 0)
    {
	close (link);

	return 1;
    }

    return 0;
}
