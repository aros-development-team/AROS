BEGIN {
    print "<HTML><HEAD>"
    print "<TITLE>AROS - The Amiga Replacement OS - AutoDoc Index</TITLE>"
    print "<BODY>\n<CENTER><P>(C) 1996 AROS - The Amiga Replacement OS</P></CENTER>\n<P><HR></P>"
    print "<DL>"
}
    { print "<DT><A HREF=\"autodocs/" tolower($0) ".html\">" $0 " ()</A>" }
END {
    print "</DL>\n</BODY>\n</HTML>"
}
