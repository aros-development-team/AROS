BEGIN {
    active=0;
}
/^\#[ \t]+BEGIN_DESC/ {
    active=1;
    match($0,/{.*}/);
    section=substr($0,RSTART+1,RLENGTH-2);
    next
}
/^\#[ \t]+END_DESC/ {
    active=0;
    next
}
/^\#/ {
    if (active)
    {
	line=$0;
	gsub(/^\#[ \t]*/,"",line);
	sections[section]=sections[section] line "\n";
    }
}
END {
    if ("makefile" in sections)
    {
	print sections["makefile"];
    }

    nfilt=split(secfilt,a_filt,/,[ \t]+/);
    for (t=1; t<=nfilt; t++)
    {
	if (a_filt[t] in sections)
	{
	    if (section=="makefile")
	    {
		# nop
	    }
	    else if (section=="target")
		print "This makefile defines the following targets:"
	    else if (section=="makevar")
		print "This makefile defines the following targets:"
	    else
		print "The section "section" contains:"
	    print "\\begin{description}\n"
	    print sections[section]
	    print "\n\\end{description}\n"
	}
    }
}
