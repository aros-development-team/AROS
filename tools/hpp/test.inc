<REM Test include file>
<EDEF NAME="filename" BEGIN="<B><I><TT>" END="</TT></I></B>">
<EDEF NAME="code" BEGIN="<PRE>" END="</PRE>">
<EDEF NAME="stdfunc" BEGIN="<FONT COLOR="red"><B>" END="</B></FONT>">
<EDEF NAME="stdfuncnc" BEGIN="<B>" END="</B>">
<EDEF NAME="ckeyword" BEGIN="<FONT COLOR="black"><B>" END="</B></FONT>">
<EDEF NAME="cppkeyword" BEGIN="<FONT COLOR="green"><B>" END="</B></FONT>">
<EDEF NAME="cppmacro" BEGIN="<FONT COLOR="green"><TT>" END="</TT></FONT>">
<EDEF NAME="ccomment" BEGIN="<FONT COLOR="green"><I>" END="</I></FONT>">
<EDEF NAME="cidentifier" BEGIN="<FONT COLOR="black"><I>" END="</I></FONT>">
<EDEF NAME="cpunctuation" BEGIN="<FONT COLOR="black"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cstring" BEGIN="<FONT COLOR="green"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cheader" BEGIN="<FONT COLOR="green"><filename>" END="</filename></FONT>">
<EDEF NAME="cvalue" BEGIN="<FONT COLOR="blue"><B><TT>" END="</B></TT></FONT>">

<DEF NAME="filename" OPTION="text"><
    IF COND="$checkdb(filenames,$text)"><
	filename><A HREF="$querydb(filenames,$text)">$text</A></filename><
    ELSE><
	filename>$text</filename><
    /IF><
/DEF>

<BDEF NAME="code" OPTION="type"><
    FILTER CMD="format -type $type -outformat $outputFormat $infilename"
	InputFile ProcessOutput><
/BDEF>

<BDEF NAME="ecode" OPTION="type"><
    FILTER CMD="format -type $type -outformat $outputFormat $infilename"
	ExpandInput InputFile ProcessOutput><
/BDEF>

<BDEF NAME="eppcode" OPTION="type"><
    FILTER CMD="format -type $type -outformat $outputFormat $infilename"
	ExpandInput ProcessInput InputFile ProcessOutput><
/BDEF>

<DEF NAME=aros_lha OPTION=type OPTION=name OPTION=reg>$type $name</DEF>
<DEF NAME=library OPTION=name><TT><EXPAND TEXT="$name"></TT></DEF>
<DEF NAME=aros_lhi OPTION=ret OPTION=name>
    <SET returntype="$ret" funcname="$name">
</DEF>
<DEF NAME=location OPTION=type OPTION=var OPTION=offset OPTION=name>
    <SET libname="$name" offset="$offset">
</DEF>
<EDEF NAME=c BEGIN="<TT>" END="</TT>">
<DEF NAME=arg OPTION=name>
    <DT><C>$name</C><DD>
</DEF>
<DEF NAME=mref OPTION=name>
    $name
</DEF>
<SET functiontmpl=amigafunc.tmpl>
