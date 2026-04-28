/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/getfont.h
*/

#ifndef GADGETS_GETFONT_H
#define GADGETS_GETFONT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define GETFONT_CLASSNAME   "gadgets/getfont.gadget"
#define GETFONT_VERSION     44

#define GETFONT_Dummy       (TAG_USER + 0x90000)

#define GETFONT_TitleText       (GETFONT_Dummy + 0x0001)
#define GETFONT_TextAttr        (GETFONT_Dummy + 0x0002)
#define GETFONT_FontName        (GETFONT_Dummy + 0x0003)
#define GETFONT_FontSize        (GETFONT_Dummy + 0x0004)
#define GETFONT_FontStyle       (GETFONT_Dummy + 0x0005)
#define GETFONT_DoStyle         (GETFONT_Dummy + 0x0006)
#define GETFONT_FixedWidthOnly  (GETFONT_Dummy + 0x0007)
#define GETFONT_MinHeight       (GETFONT_Dummy + 0x0008)
#define GETFONT_MaxHeight       (GETFONT_Dummy + 0x0009)

#define GetFontObject   NewObject(NULL, GETFONT_CLASSNAME
#define GetFontEnd      TAG_END)

#endif /* GADGETS_GETFONT_H */
