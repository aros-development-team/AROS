BEGIN {
    FS=":";
    file=ENVIRON["HOME"] "/CVSROOT/passwd.txt";

    while ((getline < file) > 0)
    {
	if ($1==""||substr($1,1,1)=="#")
	    continue;

	print $3":"$4
    }
}
