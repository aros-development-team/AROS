/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/speedbar.h
*/

#ifndef GADGETS_SPEEDBAR_H
#define GADGETS_SPEEDBAR_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define SPEEDBAR_CLASSNAME  "gadgets/speedbar.gadget"
#define SPEEDBAR_VERSION    44

#define SPEEDBAR_Dummy      (TAG_USER + 0x120000)

#define SPEEDBAR_Buttons        (SPEEDBAR_Dummy + 0x0001)
#define SPEEDBAR_Orientation    (SPEEDBAR_Dummy + 0x0002)
#define SPEEDBAR_BevelStyle     (SPEEDBAR_Dummy + 0x0003)
#define SPEEDBAR_Window         (SPEEDBAR_Dummy + 0x0004)
#define SPEEDBAR_EvenSize       (SPEEDBAR_Dummy + 0x0005)
#define SPEEDBAR_RaisedButtons  (SPEEDBAR_Dummy + 0x0006)
#define SPEEDBAR_SmallImages    (SPEEDBAR_Dummy + 0x0007)

/* SpeedBarNode attributes */
#define SBNA_Dummy          (TAG_USER + 0x120100)
#define SBNA_Image          (SBNA_Dummy + 0x0001)
#define SBNA_SelImage       (SBNA_Dummy + 0x0002)
#define SBNA_DisImage       (SBNA_Dummy + 0x0003)
#define SBNA_Disabled       (SBNA_Dummy + 0x0004)
#define SBNA_Text           (SBNA_Dummy + 0x0005)
#define SBNA_Help           (SBNA_Dummy + 0x0006)
#define SBNA_UserData       (SBNA_Dummy + 0x0007)
#define SBNA_Spacing        (SBNA_Dummy + 0x0008)
#define SBNA_Toggle         (SBNA_Dummy + 0x0009)
#define SBNA_Selected       (SBNA_Dummy + 0x000A)

#define SpeedBarObject  NewObject(NULL, SPEEDBAR_CLASSNAME
#define SpeedBarEnd     TAG_END)

#endif /* GADGETS_SPEEDBAR_H */
