<REM>
    (C) 1997-98 AROS - The Amiga Replacement OS
    $Id$

    This file contains the macros to convert the sources to HTML.
</REM>

<REM --- Environments ------------------------------------------------------->
<EDEF NAME="efilename" BEGIN="<B><I><TT>" END="</TT></I></B>">
<EDEF NAME="doc" OPTION="title" DEFAULT="" BEGIN="<begindoc title="$title">" END="<enddoc>">
<EDEF NAME="new" OPTION="date" DEFAULT="" BEGIN="<beginnew moddate="$date">" END="<endnew>">

<EDEF NAME="cstdfunc" BEGIN="<B><TT>" END="</B></TT>">
<EDEF NAME="ckeyword" BEGIN="<FONT COLOR="red"><TT>" END="</TT></FONT>">
<EDEF NAME="cppkeyword" BEGIN="<FONT COLOR="red"><TT>" END="</TT></FONT>">
<EDEF NAME="cppmacro" BEGIN="<B><TT>" END="</TT></B>">
<EDEF NAME="ccomment" BEGIN="<FONT COLOR="brown"><I>" END="</I></FONT>">
<EDEF NAME="cidentifier" BEGIN="<I>" END="</I>">
<EDEF NAME="cusertype" BEGIN="<B><I>" END="</I></B>">
<EDEF NAME="cpunctuation" BEGIN="<FONT COLOR="cyan"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cstring" BEGIN="<FONT COLOR="yellow"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cheader" BEGIN="<efilename>" END="</efilename>">
<EDEF NAME="cvalue" BEGIN="<FONT COLOR="blue"><B><TT>" END="</B></TT></FONT>">

<REM --- Counters and variables --------------------------------------------->
<SET
    ChapterCount=0
    SectionCount=0
    SubSectionCount=0
    IsNew=0
>

<REM --- Macros ------------------------------------------------------------->
<DEF NAME="filename" OPTION="text"><
    IF COND="$checkdb(filenames,$text)"><
	efilename><A HREF="$querydb(filenames,$text)">$text</A></efilename><
    ELSE><
	efilename>$text</efilename><
    /IF><
/DEF>
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
<DEF NAME=chapter OPTION=title>
    <SET ChapterCount="$expr($ChapterCount+1)"
	SectionCount=0 SubSectionCount=0>

    <H1>Chapter $ChapterCount - $title<IF COND="$IsNew"> *New*</IF></H1>
</DEF>
<DEF NAME=section OPTION=title>
    <SET SectionCount="$expr($SectionCount+1)"
	SubSectionCount=0>

    <H2>$ChapterCount.$SectionCount - $title<IF COND="$IsNew"> *New*</IF></H2>
</DEF>
<DEF NAME=subsection OPTION=title>
    <SET SubSectionCount="$expr($subsectionCount+1)">

    <H3>$ChapterCount.$SectionCount.$SubSectionCount - $title<IF COND="$IsNew"> *New*</IF></H3>
</DEF>
<DEF NAME=begindoc OPTION=title DEFAULT="">
    <HTML><HEAD>
	<TITLE>$title</TITLE>
    </HEAD>
    <BODY>
</DEF>
<DEF NAME=enddoc>
    </BODY></HTML>
</DEF>
<DEF NAME=beginnew OPTION=moddate><
    IF COND="$diffdate($moddate,$today)<30"><
	FONT COLOR="green"><
	SET IsNew=1><
    ELSE><
	SET IsNew=0><
    /IF><
/DEF>
<DEF NAME=endnew><
    IF COND="$IsNew"><
	/FONT><
	SET IsNew=0><
    /IF><
/DEF>
<DEF NAME=email OPTION=addr><A HREF="mailto:$addr"><TT>$addr</TT></A></DEF>
<DEF NAME=largepic OPTION=title OPTION=file><EXPAND TEXT="<A HREF="$file">">$title</A></DEF>
<DEF NAME=shell OPTION=text><EXPAND TEXT="<VERB TEXT="$text">"></DEF>

<REM --- Blocks ------------------------------------------------------------->
<BDEF NAME="code" OPTION="type"><
    FILTER CMD="$format -type $type -outformat $outputFormat $infilename"
	InputFile ProcessOutput><
/BDEF>

<BDEF NAME="ecode" OPTION="type"><
    FILTER CMD="$format -type $type -outformat $outputFormat $infilename"
	ExpandInput InputFile ProcessOutput><
/BDEF>

<BDEF NAME="eppcode" OPTION="type"><
    FILTER CMD="$format -type $type -outformat $outputFormat $infilename"
	ExpandInput ProcessInput InputFile ProcessOutput><
/BDEF>

