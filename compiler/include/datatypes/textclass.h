#ifndef	DATATYPES_TEXTCLASS_H
#define	DATATYPES_TEXTCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef	UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef	DATATYPES_DATATYPESCLASS_H
#   include <datatypes/datatypesclass.h>
#endif

#ifndef	LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#define	TEXTDTCLASS		"text.datatype"

/* Attributes */

#define	TDTA_Buffer	(DTA_Dummy + 300)
#define	TDTA_BufferLen	(DTA_Dummy + 301)
#define	TDTA_LineList	(DTA_Dummy + 302)
#define	TDTA_WordSelect	(DTA_Dummy + 303)
#define	TDTA_WordDelim	(DTA_Dummy + 304)
#define	TDTA_WordWrap	(DTA_Dummy + 305)

/* There is one line structure for every line of text in the document. */

struct Line
{
    struct MinNode	ln_Link;
    STRPTR		ln_Text;
    ULONG		ln_TextLen;
    UWORD		ln_XOffset;
    UWORD		ln_YOffset;
    UWORD		ln_Width;
    UWORD		ln_Height;
    UWORD		ln_Flags;
    BYTE		ln_FgPen;
    BYTE		ln_BgPen;
    ULONG		ln_Style;
    APTR		ln_Data;
};


/* ln_Flags */

#define	LNF_LF		(1L << 0)
#define	LNF_LINK	(1L << 1)
#define	LNF_OBJECT	(1L << 2)
#define	LNF_SELECTED	(1L << 3)

#define	ID_FTXT		MAKE_ID('F','T','X','T')
#define	ID_CHRS		MAKE_ID('C','H','R','S')

#endif /* DATATYPES_TEXTCLASS_H */
