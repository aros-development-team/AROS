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

#define GETFONT_TextAttr        (GETFONT_Dummy + 1)   /* Font TextAttr to display */
#define GETFONT_DoFrontPen      (GETFONT_Dummy + 2)   /* Enable front pen selection */
#define GETFONT_DoBackPen       (GETFONT_Dummy + 3)   /* Enable back pen selection */
#define GETFONT_DoStyle         (GETFONT_Dummy + 4)   /* Enable style selection */
#define GETFONT_DoDrawMode      (GETFONT_Dummy + 5)   /* Enable draw mode selection */
#define GETFONT_MinHeight       (GETFONT_Dummy + 6)   /* Minimum font height */
#define GETFONT_MaxHeight       (GETFONT_Dummy + 7)   /* Maximum font height */
#define GETFONT_FixedWidthOnly  (GETFONT_Dummy + 8)   /* Show only fixed-width fonts */
#define GETFONT_TitleText       (GETFONT_Dummy + 9)   /* ASL requester title */
#define GETFONT_Height          (GETFONT_Dummy + 10)  /* Requester height */
#define GETFONT_Width           (GETFONT_Dummy + 11)  /* Requester width */
#define GETFONT_LeftEdge        (GETFONT_Dummy + 12)  /* Requester left position */
#define GETFONT_TopEdge         (GETFONT_Dummy + 13)  /* Requester top position */
#define GETFONT_FrontPen        (GETFONT_Dummy + 14)  /* Selected front pen value */
#define GETFONT_BackPen         (GETFONT_Dummy + 15)  /* Selected back pen value */
#define GETFONT_DrawMode        (GETFONT_Dummy + 16)  /* Selected draw mode */
#define GETFONT_MaxFrontPen     (GETFONT_Dummy + 17)  /* Max colors in front palette */
#define GETFONT_MaxBackPen      (GETFONT_Dummy + 18)  /* Max colors in back palette */
#define GETFONT_ModeList        (GETFONT_Dummy + 19)  /* Substitute draw mode list */
#define GETFONT_FrontPens       (GETFONT_Dummy + 20)  /* Front pen color table */
#define GETFONT_BackPens        (GETFONT_Dummy + 21)  /* Back pen color table */
#define GETFONT_SoftStyle       (GETFONT_Dummy + 22)  /* SoftStyle for button.gadget mapping */

/* getfont.gadget methods */
#define GFONT_REQUEST   (0x600001L)

struct gfRequest
{
    ULONG MethodID;
    struct Window *gfr_Window;
};

#define gfRequestFont(obj, win) DoMethod(obj, GFONT_REQUEST, win)

#ifndef GetFontObject
#define GetFontObject   NewObject(NULL, GETFONT_CLASSNAME
#endif
#ifndef GetFontEnd
#define GetFontEnd      TAG_END)
#endif

#endif /* GADGETS_GETFONT_H */
