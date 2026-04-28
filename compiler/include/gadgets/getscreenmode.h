/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/getscreenmode.h
*/

#ifndef GADGETS_GETSCREENMODE_H
#define GADGETS_GETSCREENMODE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define GETSCREENMODE_CLASSNAME "gadgets/getscreenmode.gadget"
#define GETSCREENMODE_VERSION   44

#define GETSCREENMODE_Dummy     (TAG_USER + 0xA0000)

#define GETSCREENMODE_TitleText     (GETSCREENMODE_Dummy + 0x0001)
#define GETSCREENMODE_DisplayID     (GETSCREENMODE_Dummy + 0x0002)
#define GETSCREENMODE_DisplayWidth  (GETSCREENMODE_Dummy + 0x0003)
#define GETSCREENMODE_DisplayHeight (GETSCREENMODE_Dummy + 0x0004)
#define GETSCREENMODE_DisplayDepth  (GETSCREENMODE_Dummy + 0x0005)
#define GETSCREENMODE_OverscanType  (GETSCREENMODE_Dummy + 0x0006)
#define GETSCREENMODE_AutoScroll    (GETSCREENMODE_Dummy + 0x0007)
#define GETSCREENMODE_DoWidth       (GETSCREENMODE_Dummy + 0x0008)
#define GETSCREENMODE_DoHeight      (GETSCREENMODE_Dummy + 0x0009)
#define GETSCREENMODE_DoDepth       (GETSCREENMODE_Dummy + 0x000A)
#define GETSCREENMODE_DoOverscan    (GETSCREENMODE_Dummy + 0x000B)
#define GETSCREENMODE_DoAutoScroll  (GETSCREENMODE_Dummy + 0x000C)
#define GETSCREENMODE_PropertyFlags (GETSCREENMODE_Dummy + 0x000D)
#define GETSCREENMODE_PropertyMask  (GETSCREENMODE_Dummy + 0x000E)
#define GETSCREENMODE_MinWidth      (GETSCREENMODE_Dummy + 0x000F)
#define GETSCREENMODE_MinHeight     (GETSCREENMODE_Dummy + 0x0010)
#define GETSCREENMODE_MinDepth      (GETSCREENMODE_Dummy + 0x0011)
#define GETSCREENMODE_MaxWidth      (GETSCREENMODE_Dummy + 0x0012)
#define GETSCREENMODE_MaxHeight     (GETSCREENMODE_Dummy + 0x0013)
#define GETSCREENMODE_MaxDepth      (GETSCREENMODE_Dummy + 0x0014)

#define GetScreenModeObject NewObject(NULL, GETSCREENMODE_CLASSNAME
#define GetScreenModeEnd    TAG_END)

#endif /* GADGETS_GETSCREENMODE_H */
