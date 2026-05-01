/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/space.h
*/

#ifndef GADGETS_SPACE_H
#define GADGETS_SPACE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define SPACE_CLASSNAME     "space.gadget"
#define SPACE_VERSION       44

#define SPACE_Dummy         (REACTION_Dummy + 0x9000)

#define SPACE_MinHeight     (SPACE_Dummy + 1)  /* Gadget minimum height */
#define SPACE_MinWidth      (SPACE_Dummy + 2)  /* Gadget minimum width */
#define SPACE_MouseX        (SPACE_Dummy + 3)  /* Mouse X position within gadget (notify) */
#define SPACE_MouseY        (SPACE_Dummy + 4)  /* Mouse Y position within gadget (notify) */
#define SPACE_Transparent   (SPACE_Dummy + 5)  /* Skip background erase before redraw */
#define SPACE_AreaBox       (SPACE_Dummy + 6)  /* Inner rendering area IBox (get only) */
#define SPACE_RenderHook    (SPACE_Dummy + 7)  /* Hook called on gadget refresh */
#define SPACE_BevelStyle    (SPACE_Dummy + 8)  /* Bevel frame style (see bevel.h) */

#ifndef SpaceObject
#define SpaceObject     NewObject(NULL, SPACE_CLASSNAME
#endif
#ifndef SpaceEnd
#define SpaceEnd        TAG_END)
#endif

#endif /* GADGETS_SPACE_H */
