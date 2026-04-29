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

#define GETSCREENMODE_TitleText     (GETSCREENMODE_Dummy + 1)   /* Requester title text */
#define GETSCREENMODE_Height        (GETSCREENMODE_Dummy + 2)   /* Requester height */
#define GETSCREENMODE_Width         (GETSCREENMODE_Dummy + 3)   /* Requester width */
#define GETSCREENMODE_LeftEdge      (GETSCREENMODE_Dummy + 4)   /* Requester left position */
#define GETSCREENMODE_TopEdge       (GETSCREENMODE_Dummy + 5)   /* Requester top position */
#define GETSCREENMODE_DisplayID     (GETSCREENMODE_Dummy + 6)   /* Selected display mode ID */
#define GETSCREENMODE_DisplayWidth  (GETSCREENMODE_Dummy + 7)   /* Selected display width */
#define GETSCREENMODE_DisplayHeight (GETSCREENMODE_Dummy + 8)   /* Selected display height */
#define GETSCREENMODE_DisplayDepth  (GETSCREENMODE_Dummy + 9)   /* Selected display depth */
#define GETSCREENMODE_OverscanType  (GETSCREENMODE_Dummy + 10)  /* Selected overscan type */
#define GETSCREENMODE_AutoScroll    (GETSCREENMODE_Dummy + 11)  /* Autoscroll setting */
#define GETSCREENMODE_InfoOpened    (GETSCREENMODE_Dummy + 12)  /* Info window initially open */
#define GETSCREENMODE_InfoLeftEdge  (GETSCREENMODE_Dummy + 13)  /* Info window left position */
#define GETSCREENMODE_InfoTopEdge   (GETSCREENMODE_Dummy + 14)  /* Info window top position */
#define GETSCREENMODE_DoWidth       (GETSCREENMODE_Dummy + 15)  /* Show width gadget */
#define GETSCREENMODE_DoHeight      (GETSCREENMODE_Dummy + 16)  /* Show height gadget */
#define GETSCREENMODE_DoDepth       (GETSCREENMODE_Dummy + 17)  /* Show depth gadget */
#define GETSCREENMODE_DoOverscanType (GETSCREENMODE_Dummy + 18) /* Show overscan type gadget */
#define GETSCREENMODE_DoAutoScroll  (GETSCREENMODE_Dummy + 19)  /* Show autoscroll gadget */
#define GETSCREENMODE_PropertyFlags (GETSCREENMODE_Dummy + 20)  /* Required property flags */
#define GETSCREENMODE_PropertyMask  (GETSCREENMODE_Dummy + 21)  /* Property flags mask */
#define GETSCREENMODE_MinWidth      (GETSCREENMODE_Dummy + 22)  /* Minimum allowed width */
#define GETSCREENMODE_MaxWidth      (GETSCREENMODE_Dummy + 23)  /* Maximum allowed width */
#define GETSCREENMODE_MinHeight     (GETSCREENMODE_Dummy + 24)  /* Minimum allowed height */
#define GETSCREENMODE_MaxHeight     (GETSCREENMODE_Dummy + 25)  /* Maximum allowed height */
#define GETSCREENMODE_MinDepth      (GETSCREENMODE_Dummy + 26)  /* Minimum allowed depth */
#define GETSCREENMODE_MaxDepth      (GETSCREENMODE_Dummy + 27)  /* Maximum allowed depth */
#define GETSCREENMODE_FilterFunc    (GETSCREENMODE_Dummy + 28)  /* Hook to filter mode IDs */
#define GETSCREENMODE_CustomSMList  (GETSCREENMODE_Dummy + 29)  /* Custom DisplayMode list */

/* getscreenmode.gadget methods */
#define GSM_REQUEST (0x610001L)

struct gsmRequest
{
    ULONG MethodID;
    struct Window *gsmr_Window;
};

#define RequestScreenMode(obj, win) DoMethod(obj, GSM_REQUEST, win)

#ifndef GetScreenModeObject
#define GetScreenModeObject NewObject(NULL, GETSCREENMODE_CLASSNAME
#endif
#ifndef GetScreenModeEnd
#define GetScreenModeEnd    TAG_END)
#endif

#endif /* GADGETS_GETSCREENMODE_H */
