BEGIN {
    for (arg=1; arg<ARGC; arg++)
    {
	copyright="        Copyright © 1995-2001, The AROS Development Team. All rights reserved.";
	id="    $Id$";
	logtxt="    $Log$
	logtxt="    Revision 1.3  2001/10/30 20:28:20  chodorowski
	logtxt="    Fixed copyright notice.
	logtxt="
	logtxt="    Revision 1.2  1998/10/20 16:46:36  hkiel
	logtxt="    Amiga Research OS
	logtxt="
	logtxt="    Revision 1.1  1996/08/12 09:59:05  digulla
	logtxt="    Some scripts
	logtxt="";
	desc="    Desc:";
	lang="    Lang:";

	filename=ARGV[arg];
	out=filename ".new";

	print "Processing " filename "..."

	getline line < filename

	if (match (line, /^[ \t]*\/\*[ \t]*$/))
	{
#print "#1"
	    done=0
	    while (!done && (getline line < filename) > 0)
	    {
#print "#2 |" line "|"
		again=1;
		while (again)
		{
#print "#3 again=" again
		    again=0;
		    if (match (line, /^[ \t]*[(]C[)]/))
		    {
#print "#4"
			;
		    }
		    else if (match (line, /^[ \t]*\$Id/))
		    {
#print "#5"
			;
		    }
		    else if (match (line, /^[ \t]*\$Log/))
		    {
#print "#6"
			;
		    }
		    else if (match (line, /^[ \t]*Desc:/))
		    {
#print "#7"
			desc=line;
			while ((getline line < filename) > 0)
			{
			    if (match (line, /^[ \t]*([(]C[)]|\$Id|\$Log|Desc:|Lang:)/))
				break;
			    else if (match(line,/^[ \t]*\*\/[ \t]*$/))
			    {
				done = 1;
				break;
			    }

			    desc=desc "\n" line;
			}

			again=1;
		    }
		    else if (match (line, /^[ \t]*Lang:/))
		    {
#print "#8"
			lang=line;
			while ((getline line < filename) > 0)
			{
			    if (match (line, /^[ \t]*([(]C[)]|\$Id|\$Log|Desc:|Lang:)/))
				break;
			    else if (match(line,/^[ \t]*\*\/[ \t]*$/))
			    {
				done = 1;
				break;
			    }

			    lang=lang "\n" line;
			}

			again=1;
		    }
		    else if (match(line,/^[ \t]*\*\/[ \t]*$/))
		    {
#print "#done" line
			done=1;
		    }
		}
	    }

	    if (!done)
	    {
		print "Error: done is not set"
	    }

	    firstline="";
	}
	else
	    firstline=line;
#print "#9" line

	print "/*" > out
	print copyright >> out
	print id >> out
	print logtxt >> out
	print desc >> out
	print lang >> out
	print "*/" >> out
	if (firstline!="")
	    print firstline >> out

	while ((getline line < filename) > 0)
	{
#print "#10" line
	    print line >> out
	}

	close (out);

#print "#11" line
	system ("mv " out " " filename);
    }
}
