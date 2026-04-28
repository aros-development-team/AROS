/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/scroller.h
*/

#ifndef GADGETS_SCROLLER_H
#define GADGETS_SCROLLER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define SCROLLER_CLASSNAME      "gadgets/scroller.gadget"
#define SCROLLER_VERSION        44

#define SCROLLER_Dummy          (TAG_USER + 0xF0000)

#define SCROLLER_Total          (SCROLLER_Dummy + 0x0001)
#define SCROLLER_Visible        (SCROLLER_Dummy + 0x0002)
#define SCROLLER_Top            (SCROLLER_Dummy + 0x0003)
#define SCROLLER_Orientation    (SCROLLER_Dummy + 0x0004)
#define SCROLLER_Arrows         (SCROLLER_Dummy + 0x0005)
#define SCROLLER_ArrowDelta     (SCROLLER_Dummy + 0x0006)

#define SORIENT_HORIZ   0
#define SORIENT_VERT    1

#define ScrollerObject  NewObject(NULL, SCROLLER_CLASSNAME
#define ScrollerEnd     TAG_END)

#endif /* GADGETS_SCROLLER_H */
