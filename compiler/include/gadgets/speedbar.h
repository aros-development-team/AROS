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
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define SPEEDBAR_CLASSNAME  "speedbar.gadget"
#define SPEEDBAR_VERSION    44

#define SPEEDBAR_Dummy      (REACTION_Dummy + 0x13000)

#define SPEEDBAR_Buttons        (SPEEDBAR_Dummy + 0x0001) /* Button node list */
#define SPEEDBAR_Orientation    (SPEEDBAR_Dummy + 0x0002) /* Horiz or vert */
#define SPEEDBAR_BevelStyle     (SPEEDBAR_Dummy + 0x0003) /* Bevel style */
#define SPEEDBAR_Window         (SPEEDBAR_Dummy + 0x0004) /* Parent window */
#define SPEEDBAR_EvenSize       (SPEEDBAR_Dummy + 0x0005) /* Equal-size buttons */
#define SPEEDBAR_RaisedButtons  (SPEEDBAR_Dummy + 0x0006) /* Raised look */
#define SPEEDBAR_SmallImages    (SPEEDBAR_Dummy + 0x0007) /* Use small images */

/* SpeedBarNode tags */
#define SBNA_Dummy          (TAG_USER + 0x010000)
#define SBNA_Image          (SBNA_Dummy + 0x0001) /* Normal image */
#define SBNA_SelImage       (SBNA_Dummy + 0x0002) /* Selected image */
#define SBNA_DisImage       (SBNA_Dummy + 0x0003) /* Disabled image */
#define SBNA_Disabled       (SBNA_Dummy + 0x0004) /* Disabled state */
#define SBNA_Text           (SBNA_Dummy + 0x0005) /* Button label */
#define SBNA_Help           (SBNA_Dummy + 0x0006) /* Help/tooltip text */
#define SBNA_UserData       (SBNA_Dummy + 0x0007) /* User data pointer */
#define SBNA_Spacing        (SBNA_Dummy + 0x0008) /* Extra spacing */
#define SBNA_Toggle         (SBNA_Dummy + 0x0009) /* Toggle behavior */
#define SBNA_Selected       (SBNA_Dummy + 0x000A) /* Selected state */

#ifndef SpeedBarObject
#define SpeedBarObject  NewObject(NULL, SPEEDBAR_CLASSNAME
#endif
#ifndef SpeedBarEnd
#define SpeedBarEnd     TAG_END)
#endif

#endif /* GADGETS_SPEEDBAR_H */
