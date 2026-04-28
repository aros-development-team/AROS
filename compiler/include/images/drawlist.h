/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/drawlist.h
*/

#ifndef IMAGES_DRAWLIST_H
#define IMAGES_DRAWLIST_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define DRAWLIST_CLASSNAME  "images/drawlist.image"
#define DRAWLIST_VERSION    44

#define DRAWLIST_Dummy      (TAG_USER + 0x170000)

#define DRAWLIST_Directives     (DRAWLIST_Dummy + 0x0001)
#define DRAWLIST_NumDirectives  (DRAWLIST_Dummy + 0x0002)

/* Drawlist directive commands */
#define DLD_END         0
#define DLD_MOVE        1
#define DLD_DRAW        2
#define DLD_FILL        3
#define DLD_NOPEN       4
#define DLD_FILLPEN     5
#define DLD_BPEN        6
#define DLD_AFPT        7
#define DLD_AFPTSIZE    8

/* DrawList directive structure */
struct DrawList
{
    WORD dl_Command;
    WORD dl_X;
    WORD dl_Y;
};

#define DrawListObject  NewObject(NULL, DRAWLIST_CLASSNAME
#define DrawListEnd     TAG_END)

#endif /* IMAGES_DRAWLIST_H */
