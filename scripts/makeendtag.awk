BEGIN {
    stderr="/dev/stderr";

    ename=tolower(lib) "_endtag.c";

    print "/*" > ename;
    print "        (C) 1995-2001 AROS - The Amiga Research OS" >> ename
    print "        *** Automatic generated file. Do not edit ***" >> ename
    print "        Desc: Resident endskip for " lib >> ename
    print "        Lang: english" >> ename
    print "*/" >> ename;
    print "\nconst char " base "_end = 0;" >> ename;
}
