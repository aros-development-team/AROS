<REM>
    (C) 1997-98 AROS - The Amiga Replacement OS
    $Id$

    This file contains the macros to convert the sources to HTML.
</REM>

<REM --- Environments ------------------------------------------------------->
<EDEF NAME="efilename" BEGIN="" END="">
<EDEF NAME="doc" OPTION="title" DEFAULT="" BEGIN="/* $title */" END="/* END */">
<EDEF NAME="new" OPTION="date" DEFAULT="" BEGIN="/* NEW $date */" END="/* ENDNEW */">

<EDEF NAME="cstdfunc" BEGIN="" END="">
<EDEF NAME="ckeyword" BEGIN="" END="">
<EDEF NAME="cppkeyword" BEGIN="" END="">
<EDEF NAME="cppmacro" BEGIN="" END="">
<EDEF NAME="ccomment" BEGIN="" END="">
<EDEF NAME="cidentifier" BEGIN="" END="">
<EDEF NAME="cusertype" BEGIN="" END="">
<EDEF NAME="cpunctuation" BEGIN="" END="">
<EDEF NAME="cstring" BEGIN="" END="">
<EDEF NAME="cheader" BEGIN="" END="">
<EDEF NAME="cvalue" BEGIN="" END="">

<REM --- Counters and variables --------------------------------------------->
<SET
    ChapterCount=0
    SectionCount=0
    SubSectionCount=0
    IsNew=0
    numargs=0
    first=1
>

<REM --- Macros ------------------------------------------------------------->
<DEF NAME="filename" OPTION="text">$filename</DEF>
<DEF NAME=aros_lha OPTION=type OPTION=name OPTION=reg>
    AROS_LHA($type,$name,$reg)
</DEF>
<DEF NAME=library OPTION=name><TT><EXPAND TEXT="$name"></TT></DEF>

<REM --- Blocks ------------------------------------------------------------->
<BDEF NAME="code" OPTION="type"><
    FILTER CMD="sed -f toc.filter $infilename"
	InputFile><
/BDEF>

<BDEF NAME="ecode" OPTION="type"><
    FILTER CMD="sed -f toc.filter $infilename"
	ExpandInput InputFile><
/BDEF>

