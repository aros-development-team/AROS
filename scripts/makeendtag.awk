BEGIN {
    stderr="/dev/stderr";

    ename=tolower(lib) "_endtag.c";
}
END {
    print "/*" > ename;
    print "        (C) 1995-96 AROS - The Amiga Replacement OS" >> ename
    print "        *** Automatic generated file. Do not edit ***" >> ename
    print "        Desc: Resident endskip for " lib >> ename
    print "        Lang: english" >> ename
    print "*/" >> ename;
    print "\nconst char " lib "_end = 0;" >> ename;
}
