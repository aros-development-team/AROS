/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/getscreenmode.h
*/

#ifndef GADGETS_GETSCREENMODE_H
#define GADGETS_GETSCREENMODE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define GETSCREENMODE_CLASSNAME "getscreenmode.gadget"
#define GETSCREENMODE_VERSION   44

#define GETSCREENMODE_Dummy     (REACTION_Dummy + 0x41000)

#define GETSCREENMODE_TitleText     (GETSCREENMODE_Dummy + 0x0001) /* Requester title */
#define GETSCREENMODE_DisplayID     (GETSCREENMODE_Dummy + 0x0002) /* Selected mode ID */
#define GETSCREENMODE_DisplayWidth  (GETSCREENMODE_Dummy + 0x0003) /* Selected width */
#define GETSCREENMODE_DisplayHeight (GETSCREENMODE_Dummy + 0x0004) /* Selected height */
#define GETSCREENMODE_DisplayDepth  (GETSCREENMODE_Dummy + 0x0005) /* Selected depth */
#define GETSCREENMODE_OverscanType  (GETSCREENMODE_Dummy + 0x0006) /* Overscan type */
#define GETSCREENMODE_AutoScroll    (GETSCREENMODE_Dummy + 0x0007) /* Autoscroll flag */
#define GETSCREENMODE_DoWidth       (GETSCREENMODE_Dummy + 0x0008) /* Show width gadget */
#define GETSCREENMODE_DoHeight      (GETSCREENMODE_Dummy + 0x0009) /* Show height gadget */
#define GETSCREENMODE_DoDepth       (GETSCREENMODE_Dummy + 0x000A) /* Show depth gadget */
#define GETSCREENMODE_DoOverscan    (GETSCREENMODE_Dummy + 0x000B) /* Show overscan gadget */
#define GETSCREENMODE_DoAutoScroll  (GETSCREENMODE_Dummy + 0x000C) /* Show autoscroll gadget */
#define GETSCREENMODE_PropertyFlags (GETSCREENMODE_Dummy + 0x000D) /* Required mode properties */
#define GETSCREENMODE_PropertyMask  (GETSCREENMODE_Dummy + 0x000E) /* Property filter mask */
#define GETSCREENMODE_MinWidth      (GETSCREENMODE_Dummy + 0x000F) /* Min allowed width */
#define GETSCREENMODE_MinHeight     (GETSCREENMODE_Dummy + 0x0010) /* Min allowed height */
#define GETSCREENMODE_MinDepth      (GETSCREENMODE_Dummy + 0x0011) /* Min allowed depth */
#define GETSCREENMODE_MaxWidth      (GETSCREENMODE_Dummy + 0x0012) /* Max allowed width */
#define GETSCREENMODE_MaxHeight     (GETSCREENMODE_Dummy + 0x0013) /* Max allowed height */
#define GETSCREENMODE_MaxDepth      (GETSCREENMODE_Dummy + 0x0014) /* Max allowed depth */

#ifndef GetScreenModeObject
#define GetScreenModeObject NewObject(NULL, GETSCREENMODE_CLASSNAME
#endif
#ifndef GetScreenModeEnd
#define GetScreenModeEnd    TAG_END)
#endif

#endif /* GADGETS_GETSCREENMODE_H */
