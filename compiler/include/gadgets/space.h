/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/space.h
*/

#ifndef GADGETS_SPACE_H
#define GADGETS_SPACE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define SPACE_CLASSNAME     "gadgets/space.gadget"
#define SPACE_VERSION       44

#define SPACE_Dummy         (TAG_USER + 0x110000)

#define SPACE_MinWidth      (SPACE_Dummy + 0x0001)
#define SPACE_MinHeight     (SPACE_Dummy + 0x0002)
#define SPACE_BevelStyle    (SPACE_Dummy + 0x0003)
#define SPACE_Transparent   (SPACE_Dummy + 0x0004)

#define SpaceObject     NewObject(NULL, SPACE_CLASSNAME
#define SpaceEnd        TAG_END)

#endif /* GADGETS_SPACE_H */
