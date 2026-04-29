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

#define SPACE_MinWidth      (SPACE_Dummy + 0x0001) /* Minimum width */
#define SPACE_MinHeight     (SPACE_Dummy + 0x0002) /* Minimum height */
#define SPACE_BevelStyle    (SPACE_Dummy + 0x0003) /* Bevel style */
#define SPACE_Transparent   (SPACE_Dummy + 0x0004) /* No background fill */

#ifndef SpaceObject
#define SpaceObject     NewObject(NULL, SPACE_CLASSNAME
#endif
#ifndef SpaceEnd
#define SpaceEnd        TAG_END)
#endif

#endif /* GADGETS_SPACE_H */
