/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/glyph.h
*/

#ifndef IMAGES_GLYPH_H
#define IMAGES_GLYPH_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define GLYPH_CLASSNAME     "images/glyph.image"
#define GLYPH_VERSION       44

#define GLYPH_Dummy         (TAG_USER + 0x180000)

#define GLYPH_Glyph         (GLYPH_Dummy + 0x0001)
#define GLYPH_SoftStyle     (GLYPH_Dummy + 0x0002)

/* Glyph types */
#define GLYPH_POPFILE       0
#define GLYPH_POPFONT       1
#define GLYPH_POPUP         2
#define GLYPH_POPDRAWER     3
#define GLYPH_POPSCREEN     4
#define GLYPH_POPTIME       5
#define GLYPH_ARROWLEFT     6
#define GLYPH_ARROWRIGHT    7
#define GLYPH_ARROWUP       8
#define GLYPH_ARROWDOWN     9
#define GLYPH_CHECKBOX      10
#define GLYPH_RADIOBUTTON   11
#define GLYPH_UPARROW       12
#define GLYPH_DOWNARROW     13
#define GLYPH_LEFTARROW     14
#define GLYPH_RIGHTARROW    15

#define GlyphObject     NewObject(NULL, GLYPH_CLASSNAME
#define GlyphEnd        TAG_END)

#endif /* IMAGES_GLYPH_H */
