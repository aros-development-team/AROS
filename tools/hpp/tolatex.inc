<REM>
    (C) 1997-98 AROS - The Amiga Replacement OS
    $Id$

    This file contains the macros to convert the sources to HTML.
</REM>

<REM --- Environments ------------------------------------------------------->
<EDEF NAME="B" BEGIN="{\bfseries " END="}">
<EDEF NAME="I" BEGIN="{\itshape " END="}">
<EDEF NAME="TT" BEGIN="{\ttfamily " END="}">
<EDEF NAME="FONT" OPTION=color DEFAULT="" BEGIN="" END="">
<EDEF NAME=ul BEGIN="\begin{itemize}" END="\end{itemize}">
<EDEF NAME=dl BEGIN="\begin{description}" END="\end{description}">
<EDEF NAME=a OPTION=href DEFAULT="" OPTION=name DEFAULT="" BEGIN="" END="">
<EDEF NAME=center BEGIN="\begin{center}" END="\end{center}">
<EDEF NAME=c BEGIN="{\ttfamily " END="}">

<EDEF NAME="efilename" BEGIN="<B><I><TT>" END="</TT></I></B>">
<EDEF NAME="doc" OPTION="title" DEFAULT="" BEGIN="<begindoc title="$title">" END="<enddoc>">
<EDEF NAME="new" OPTION="date" DEFAULT="" BEGIN="<beginnew moddate="$date">" END="<endnew>">

<EDEF NAME="cstdfunc" BEGIN="<B>" END="</B>">
<EDEF NAME="ckeyword" BEGIN="<FONT COLOR="red"><TT>" END="</TT></FONT>">
<EDEF NAME="cppkeyword" BEGIN="<FONT COLOR="green"><TT>" END="</TT></FONT>">
<EDEF NAME="cppmacro" BEGIN="<TT>" END="</TT>">
<EDEF NAME="ccomment" BEGIN="<FONT COLOR="green"><I>" END="</I></FONT>">
<EDEF NAME="cidentifier" BEGIN="<I>" END="</I>">
<EDEF NAME="cusertype" BEGIN="<B><I>" END="</I></B>">
<EDEF NAME="cpunctuation" BEGIN="<FONT COLOR="orange"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cstring" BEGIN="<FONT COLOR="yellow"><B><TT>" END="</TT></B></FONT>">
<EDEF NAME="cheader" BEGIN="<efilename>" END="</efilename>">
<EDEF NAME="cvalue" BEGIN="<FONT COLOR="blue"><B><TT>" END="</B></TT></FONT>">

<REM --- Counters and variables --------------------------------------------->
<SET
    RefCount=0
    IsNew=0
>

<REM --- Macros ------------------------------------------------------------->
<DEF NAME=HR>\hrule
</DEF>
<DEF NAME=DT>\item[</DEF>
<DEF NAME=DD>]\ \\</DEF>
<DEF NAME=p>\par
</DEF>
<DEF NAME=br>\\</DEF>
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
    \item[<C>$name</C>]
</DEF>
<DEF NAME=mref OPTION=name>
    $name
</DEF>
<DEF NAME=chapter OPTION=title>
    \chapter{$title}
</DEF>
<DEF NAME=section OPTION=title>\section{$title}</DEF>
<DEF NAME=subsection OPTION=title>
    \subsection{$title}
</DEF>
<DEF NAME=begindoc OPTION=title DEFAULT="">
    \documentclass{book}
    %\usepackage[draft,dvips]{graphicx}
    \usepackage[final,dvips]{graphicx}
    \usepackage[DVIps]{changebar}
    \begin{document}
</DEF>
<DEF NAME=enddoc>
    \end{document}
</DEF>
<DEF NAME=beginnew OPTION=moddate><
    IF COND="$diffdate($moddate,$today)<30">
	\begin{changebar}<
	SET IsNew=1><
    ELSE><
	SET IsNew=0><
    /IF><
/DEF>
<DEF NAME=endnew><
    IF COND="$IsNew">\end{changebar}<
/DEF>
<DEF NAME=email OPTION=addr><TT>$addr</TT></A></DEF>
<DEF NAME=largepic OPTION=title OPTION=file>
    <SET RefCount="$expr($RefCount+1)">
    See ``$title'' on page \pageref{lab$RefCount}.
    \begin{figure}
    \begin{center}
    \includegraphics[scale=.5]{$file.eps}
    \label{lab$RefCount}
    \caption{$title}
    \end{center}
    \end{figure}
</DEF>
<DEF NAME=shell OPTION=text>\verb§$text§</DEF>
<DEF NAME=li>\item</DEF>

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

