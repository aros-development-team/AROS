/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/drawlist.h
*/

#ifndef IMAGES_DRAWLIST_H
#define IMAGES_DRAWLIST_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define DRAWLIST_CLASSNAME  "drawlist.image"
#define DRAWLIST_VERSION    44

#define DRAWLIST_Dummy      (REACTION_Dummy + 0x17000)

#define DRAWLIST_Directives     (DRAWLIST_Dummy + 0x0001) /* Array of DrawList structs */
#define DRAWLIST_NumDirectives  (DRAWLIST_Dummy + 0x0002) /* Directive count */

/* Drawing commands */
#define DLD_END         0
#define DLD_MOVE        1
#define DLD_DRAW        2
#define DLD_FILL        3
#define DLD_NOPEN       4
#define DLD_FILLPEN     5
#define DLD_BPEN        6
#define DLD_AFPT        7
#define DLD_AFPTSIZE    8

/* DrawList directive entry */
struct DrawList
{
    WORD dl_Command;
    WORD dl_X;
    WORD dl_Y;
};

#ifndef DrawListObject
#define DrawListObject  NewObject(NULL, DRAWLIST_CLASSNAME
#endif
#ifndef DrawListEnd
#define DrawListEnd     TAG_END)
#endif

#endif /* IMAGES_DRAWLIST_H */
