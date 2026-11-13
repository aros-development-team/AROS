/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/scroller.h
*/

#ifndef GADGETS_SCROLLER_H
#define GADGETS_SCROLLER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#define SCROLLER_CLASSNAME      "scroller.gadget"
#define SCROLLER_VERSION        44

#define SCROLLER_Dummy          (REACTION_Dummy + 0x0005000)

#define SCROLLER_Top            (SCROLLER_Dummy + 1)   /* Top position (WORD) */
#define SCROLLER_Visible        (SCROLLER_Dummy + 2)   /* Visible portion of total (WORD) */
#define SCROLLER_Total          (SCROLLER_Dummy + 3)   /* Total scroller range (WORD) */
#define SCROLLER_Orientation    (SCROLLER_Dummy + 4)   /* Horizontal or vertical (WORD) */
#define SCROLLER_Arrows         (SCROLLER_Dummy + 5)   /* Show arrow buttons (BOOL) */
#define SCROLLER_Stretch        (SCROLLER_Dummy + 6)   /* Auto-expand total (BOOL) */
#define SCROLLER_ArrowDelta     (SCROLLER_Dummy + 7)   /* Arrow click increment (WORD) */
#define SCROLLER_SignalTask      (SCROLLER_Dummy + 10)  /* Task to signal while active */
#define SCROLLER_SignalTaskBit   (SCROLLER_Dummy + 11)  /* Signal bit to use (ULONG) */

/* Orientation modes */
#define SORIENT_HORIZ           FREEHORIZ
#define SORIENT_VERT            FREEVERT

#define SCROLLER_HORIZONTAL     SORIENT_HORIZ
#define SCROLLER_VERTICAL       SORIENT_VERT

#ifndef ScrollerObject
#define ScrollerObject  NewObject(NULL, SCROLLER_CLASSNAME
#endif
#ifndef ScrollerEnd
#define ScrollerEnd     TAG_END)
#endif

#endif /* GADGETS_SCROLLER_H */
