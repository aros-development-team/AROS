
	********************
	* ReqTools library *
	********************

	SECTION "reqtools",CODE

	INCLUDE	"exec/types.i"
	INCLUDE	"exec/nodes.i"
	INCLUDE	"exec/resident.i"
	INCLUDE	"exec/libraries.i"

	INCLUDE "reqtools_rev.i"

	XREF Init

	XDEF DataTable

myINITBYTE	MACRO	* &offset,&value
	DC.B	$e0
	DC.B	0
	DC.W	\1
	DC.B	\2
	DC.B	0
	ENDM

myINITWORD	MACRO	* &offset,&value
	DC.B	$d0
	DC.B	0
	DC.W	\1
	DC.W	\2
	ENDM

myINITLONG	MACRO	* &offset,&value
	DC.B	$c0
	DC.B	0
	DC.W	\1
	DC.L	\2
	ENDM
      
myINITSTRUCT  MACRO   * &size,&offset,&value,&count
	DS.W    0
	IFC	    '\4',''
COUNT\@	    SET	    0
	ENDC
	IFNC    '\4',''
COUNT\@	    SET	    \4
	ENDC
CMD\@	    SET	    (((\1)<<4)!COUNT\@)
	IFLE    (\2)-255
	DC.B    (CMD\@)!$80
	DC.B    \2
	MEXIT
	ENDC
	DC.B    CMD\@!$0C0
	DC.B    (((\2)>>16)&$0FF)
	DC.W    ((\2)&$0FFFF)
	ENDM

Start:
	moveq #-1,d0
	rts

reqtoolsname:	dc.b "reqtools.library",0
idstring:	VSTRING
	cnop 0,2

	; Romtag structure
Romtag:
	dc.w RTC_MATCHWORD
	dc.l Romtag
	dc.l EndCode
	dc.b RTF_AUTOINIT
	dc.b VERSION
	dc.b NT_LIBRARY
	dc.b 0
	dc.l reqtoolsname
	dc.l idstring
	dc.l Init

DataTable:
	myINITBYTE	LN_TYPE,NT_LIBRARY
	myINITLONG	LN_NAME,reqtoolsname
	myINITBYTE	LIB_FLAGS,LIBF_SUMUSED+LIBF_CHANGED
	myINITWORD	LIB_VERSION,VERSION
	myINITWORD	LIB_REVISION,REVISION
	myINITLONG	LIB_IDSTRING,idstring
	dc.l 0

EndCode:

	END
