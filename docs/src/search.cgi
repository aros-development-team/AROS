#!/bin/sh

search=`echo "$QUERY_STRING" | cut -d= -f2-`

echo "Content-type: text/html"
echo ""
echo "<HTML>"
echo "<TITLE>AROS - The Amiga Research OS - Search results for $search</TITLE>"
echo "<H1>Search results for <I>$search</I></H1>"
#echo "<PRE>"
#pwd
#printenv
#echo "</PRE>"

glimpse -H /home/digulla/AROS/docs/html -i $search | \
sort | \
gawk 'BEGIN { IGNORECASE=1; count=0; } \
 { \
    if (!count) print "<DL>"; \
    count ++; \
    match($0,/[^:]+:/);
    file=substr($0,RSTART,RLENGTH-1); \
    gsub(/\/home\/digulla\/AROS\/docs\/html\//,"",file); \
    printf ("<DT><A HREF=\"../%s\">%s</A>\n<DD>", file, file); \
    line=substr($0,RSTART+RLENGTH+1); \
    gsub("^[ \t]+","",line); \
    gsub("[ \t]+$","",line); \
    rest=line; \
    line=""; \
    inhref=0; \
							    \
    while (rest!="")                                        \
    {							    \
	if (match(rest,/<[^>]+>/))                        \
	{						    \
	    prefix=substr(rest,1,RSTART-1);                 \
	    tag=substr(rest,RSTART,RLENGTH);                \
	    rest=substr(rest,RSTART+RLENGTH);               \
							    \
	    gsub(/'$search'/,"<B>&</B>",prefix);            \
	    line=line prefix;				    \
	    if (tag=="</A>" && inhref)                      \
	    {						    \
		line=line tag;				    \
		inhref=0;				    \
	    }						    \
	    else if (match(tag,/^<A[ \t]+HREF/))            \
	    {						    \
		inhref=1;				    \
		line=line tag;				    \
	    }						    \
	}						    \
	else						    \
	{						    \
	    gsub(/'$search'/,"<B>&</B>",rest);              \
	    line=line rest;				    \
	    rest="";                                        \
	}						    \
    }							    \
    \
    if (inhref) line=line"...</A>"; \
    print "<DD>..." line "..." \
} \
END { if (!count) print "No matches"; else print "</DL>"; }'
echo "</DL>"
echo "</BODY></HTML>"

