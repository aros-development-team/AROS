<REM Test include file>
<EDEF NAME="filename" BEGIN="<B><I><TT>" END="</TT></I></B>">
<EDEF NAME="code" BEGIN="<PRE>" END="</PRE>">
<EDEF NAME="stdfunc" BEGIN="<FONT COLOR="orange"><B>" END="</B></FONT>">
<EDEF NAME="ckeyword" BEGIN="<FONT COLOR="black"><B>" END="</B></FONT>">
<EDEF NAME="cppkeyword" BEGIN="<FONT COLOR="green"><B>" END="</B></FONT>">
<EDEF NAME="ccomment" BEGIN="<FONT COLOR="green"><I>" END="</I></FONT>">
<EDEF NAME="cidentifier" BEGIN="<FONT COLOR="black"><I>" END="</I></FONT>">
<EDEF NAME="cpunctuation" BEGIN="<FONT COLOR="black"><TT>" END="</TT></FONT>">
<EDEF NAME="cstring" BEGIN="<FONT COLOR="green"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cheader" BEGIN="<FONT COLOR="green"><filename>" END="</filename></FONT>">

<DEF NAME="filename" OPTION="text"><
    IF COND="$checkdb(filenames,$text)"><
	filename><A HREF="$querydb(filenames,$text)">$text</A></filename><
    ELSE><
	filename>$text</filename><
    /IF><
/DEF>

<BDEF NAME="code" OPTION="type">
<REM><	 ENV NAME="code"></REM><
    FILTER CMD="format -type $type -outformat $outputFormat $infilename"
	VerbatimInput InputFile ProcessOutput><REM><
    /ENV></REM><
/BDEF>
