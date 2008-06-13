#ifndef PREFS_FONT_H
#define PREFS_FONT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Font prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_TEXT_H
#   include <graphics/text.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif


#define ID_FONT MAKE_ID('F','O','N','T')

/* The maximum length the name of a font may have. */
#define FONTNAMESIZE 128

struct FontPrefs
{
    LONG            fp_Reserved[3]; /* PRIVATE */
    UWORD           fp_Reserved2;   /* PRIVATE */
    UWORD           fp_Type;        /* see below */
    UBYTE           fp_FrontPen;
    UBYTE           fp_BackPen;
    UBYTE           fp_DrawMode;
    struct TextAttr fp_TextAttr;
    BYTE            fp_Name[FONTNAMESIZE];
};

/* Values for fp_Type */
#define FP_WBFONT     0
#define FP_SYSFONT    1
#define FP_SCREENFONT 2

#endif /* PREFS_FONT_H */
