BEGIN {
    FS=":";
    file=ENVIRON["HOME"] "/CVSROOT/passwd.txt";

    print "\\begin{itemize}"

    while ((getline < file) > 0)
    {
	if ($1==""||substr($1,1,1)=="#")
	    continue;

	if ($5!="")
	    print "\\item "$3" ("$5")"
	else
	    print "\\item "$3
    }

    print "\\end{itemize}"
}
