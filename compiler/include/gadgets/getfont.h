/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/getfont.h
*/

#ifndef GADGETS_GETFONT_H
#define GADGETS_GETFONT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define GETFONT_CLASSNAME   "getfont.gadget"
#define GETFONT_VERSION     44

#define GETFONT_Dummy       (REACTION_Dummy + 0x40000)

#define GETFONT_TitleText       (GETFONT_Dummy + 0x0001) /* Requester title */
#define GETFONT_TextAttr        (GETFONT_Dummy + 0x0002) /* TextAttr result */
#define GETFONT_FontName        (GETFONT_Dummy + 0x0003) /* Selected font name */
#define GETFONT_FontSize        (GETFONT_Dummy + 0x0004) /* Selected size */
#define GETFONT_FontStyle       (GETFONT_Dummy + 0x0005) /* Selected style flags */
#define GETFONT_DoStyle         (GETFONT_Dummy + 0x0006) /* Show style options */
#define GETFONT_FixedWidthOnly  (GETFONT_Dummy + 0x0007) /* Only monospace fonts */
#define GETFONT_MinHeight       (GETFONT_Dummy + 0x0008) /* Minimum font size */
#define GETFONT_MaxHeight       (GETFONT_Dummy + 0x0009) /* Maximum font size */

#ifndef GetFontObject
#define GetFontObject   NewObject(NULL, GETFONT_CLASSNAME
#endif
#ifndef GetFontEnd
#define GetFontEnd      TAG_END)
#endif

#endif /* GADGETS_GETFONT_H */
