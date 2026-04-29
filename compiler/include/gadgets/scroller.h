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

#define SCROLLER_CLASSNAME      "scroller.gadget"
#define SCROLLER_VERSION        44

#define SCROLLER_Dummy          (REACTION_Dummy + 0x0005000)

#define SCROLLER_Total          (SCROLLER_Dummy + 0x0001) /* Total units */
#define SCROLLER_Visible        (SCROLLER_Dummy + 0x0002) /* Visible portion */
#define SCROLLER_Top            (SCROLLER_Dummy + 0x0003) /* Top position */
#define SCROLLER_Orientation    (SCROLLER_Dummy + 0x0004) /* Horiz or vert */
#define SCROLLER_Arrows         (SCROLLER_Dummy + 0x0005) /* Show arrow buttons */
#define SCROLLER_ArrowDelta     (SCROLLER_Dummy + 0x0006) /* Arrow click step */

#define SORIENT_HORIZ   0
#define SORIENT_VERT    1

#ifndef ScrollerObject
#define ScrollerObject  NewObject(NULL, SCROLLER_CLASSNAME
#endif
#ifndef ScrollerEnd
#define ScrollerEnd     TAG_END)
#endif

#endif /* GADGETS_SCROLLER_H */
