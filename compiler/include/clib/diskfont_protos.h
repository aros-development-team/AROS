#ifndef CLIB_DISKFONT_PROTOS_H
#define CLIB_DISKFONT_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for diskfont.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP3(LONG, AvailFonts,
    AROS_LPA(STRPTR, buffer, A0),
    AROS_LPA(LONG  , bufBytes, D0),
    AROS_LPA(LONG  , flags, D1),
    struct Library *, DiskfontBase, 6, Diskfont)

AROS_LP1(void, DisposeFontContents,
    AROS_LPA(struct FontContentsHeader *, fontContentsHeader, A1),
    struct Library *, DiskfontBase, 8, Diskfont)

AROS_LP2(struct FontContentsHeader *, NewFontContents,
    AROS_LPA(BPTR  , fontsLock, A0),
    AROS_LPA(STRPTR, fontName, A1),
    struct Library *, DiskfontBase, 7, Diskfont)

AROS_LP2(struct DiskFont *, NewScaledDiskFont,
    AROS_LPA(struct TextFont *, sourceFont, A0),
    AROS_LPA(struct TextAttr *, destTextAttr, A1),
    struct Library *, DiskfontBase, 9, Diskfont)

AROS_LP1(struct TextFont *, OpenDiskFont,
    AROS_LPA(struct TextAttr *, textAttr, A0),
    struct Library *, DiskfontBase, 5, Diskfont)


#endif /* CLIB_DISKFONT_PROTOS_H */
