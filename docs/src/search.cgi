#!/bin/sh

read search
search=`echo "$search" | cut -d= -f2-`

echo "Content-type: text/html"
echo ""
echo "<HTML>"
echo "<TITLE>AROS - The Amiga Replacement OS - Search results for $search</TITLE>"
echo "<H1>Search results for <I>$search</I></H1>"

files=`find .. -type f -exec grep -iq $search "{}" \; -print | sort`
#files="../adoc_index.html ../autodocs/clonerastport.html"

if [ -z "$files" ]; then
    echo "Nothing found"
else
    for fn in $files ; do
	file=`echo $fn | cut -d/ -f2-`
	echo "<A HREF=\"$fn\">$file</A><BR><DL>"
	gawk 'BEGIN { IGNORECASE=1; } \
	    /'$search'/ { \
		gsub("^[ \t]+",""); \
		gsub("[ \t]+$",""); \
		rest=$0; \
		line=""; \
		inhref=0; \
									\
		while (rest!="")                                        \
		{							\
		    if (match(rest,/<[^>]+>/))                        \
		    {							\
			prefix=substr(rest,1,RSTART-1);                 \
			tag=substr(rest,RSTART,RLENGTH);                \
			rest=substr(rest,RSTART+RLENGTH);               \
									\
			gsub(/'$search'/,"<B>&</B>",prefix);            \
			line=line prefix;				\
			if (tag=="</A>" && inhref)                      \
			{						\
			    line=line tag;				\
			    inhref=0;					\
			}						\
			else if (match(tag,/^<A[ \t]+HREF/))            \
			{						\
			    inhref=1;					\
			    line=line tag;				\
			}						\
		    }							\
		    else						\
		    {							\
			gsub(/'$search'/,"<B>&</B>",rest);              \
			line=line rest; 				\
			rest="";                                        \
		    }							\
		}							\
		\
		if (inhref) line=line"...</A>"; \
		print "<DD>..." line "..." }' $fn
	echo "</DL>"
    done
fi

echo "</BODY></HTML>"

