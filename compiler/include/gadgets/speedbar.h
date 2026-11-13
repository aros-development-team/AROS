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

/* SpeedBarNode attributes */
#define SBNA_Dummy          (TAG_USER + 0x010000)

#define SBNA_Left           (SBNA_Dummy + 1)   /* Left offset of button */
#define SBNA_Top            (SBNA_Dummy + 2)   /* Top offset of button */
#define SBNA_Width          (SBNA_Dummy + 3)   /* Button width */
#define SBNA_Height         (SBNA_Dummy + 4)   /* Button height */
#define SBNA_UserData       (SBNA_Dummy + 5)   /* User data pointer */
#define SBNA_Enabled        (SBNA_Dummy + 6)   /* Button enabled state */
#define SBNA_Spacing        (SBNA_Dummy + 7)   /* Spacing from previous button */
#define SBNA_Highlight      (SBNA_Dummy + 8)   /* Highlight mode (see SBH_) */
#define SBNA_Image          (SBNA_Dummy + 9)   /* Normal state image */
#define SBNA_SelImage       (SBNA_Dummy + 10)  /* Selected state image */
#define SBNA_Help           (SBNA_Dummy + 11)  /* Help text string */
#define SBNA_Toggle         (SBNA_Dummy + 12)  /* Toggle button mode */
#define SBNA_Selected       (SBNA_Dummy + 13)  /* Toggle button state */
#define SBNA_MXGroup        (SBNA_Dummy + 14)  /* Mutual exclusion group (implies toggle) */
#define SBNA_Disabled       (SBNA_Dummy + 15)  /* Disabled with ghost pattern */

/* SpeedBarNode highlight modes */
#define SBH_NONE        0
#define SBH_BACKFILL    1
#define SBH_RECESS      2
#define SBH_IMAGE       3

/* SpeedBar gadget attributes */
#define SPEEDBAR_Dummy      (REACTION_Dummy + 0x13000)

#define SPEEDBAR_Buttons        (SPEEDBAR_Dummy + 1)   /* Button list */
#define SPEEDBAR_Orientation    (SPEEDBAR_Dummy + 2)   /* Horizontal or vertical */
#define SPEEDBAR_Background     (SPEEDBAR_Dummy + 3)   /* Background color */
#define SPEEDBAR_Window         (SPEEDBAR_Dummy + 4)   /* Window for help display */
#define SPEEDBAR_StrumBar       (SPEEDBAR_Dummy + 5)   /* Allow strumming of buttons */
#define SPEEDBAR_OnButton       (SPEEDBAR_Dummy + 6)   /* Turn on button by ID */
#define SPEEDBAR_OffButton      (SPEEDBAR_Dummy + 7)   /* Turn off button by ID */
#define SPEEDBAR_ScrollLeft     (SPEEDBAR_Dummy + 8)   /* Scroll buttons left */
#define SPEEDBAR_ScrollRight    (SPEEDBAR_Dummy + 9)   /* Scroll buttons right */
#define SPEEDBAR_Top            (SPEEDBAR_Dummy + 10)  /* First visible button */
#define SPEEDBAR_Visible        (SPEEDBAR_Dummy + 11)  /* Number of visible buttons */
#define SPEEDBAR_Total          (SPEEDBAR_Dummy + 12)  /* Total buttons in list */
#define SPEEDBAR_Help           (SPEEDBAR_Dummy + 13)  /* Help text string */
#define SPEEDBAR_BevelStyle     (SPEEDBAR_Dummy + 14)  /* Bevel box style */
#define SPEEDBAR_Selected       (SPEEDBAR_Dummy + 15)  /* Last selected node number */
#define SPEEDBAR_SelectedNode   (SPEEDBAR_Dummy + 16)  /* Last selected node pointer */
#define SPEEDBAR_EvenSize       (SPEEDBAR_Dummy + 17)  /* Size all buttons equally */

/* Orientation modes */
#define SBORIENT_HORIZ  0
#define SBORIENT_VERT   1

/* Obsolete names - do not use */
#define SPEEDBAR_HORIZONTAL SBORIENT_HORIZ
#define SPEEDBAR_VERTICAL   SBORIENT_VERT

#ifndef SpeedBarObject
#define SpeedBarObject  NewObject(NULL, SPEEDBAR_CLASSNAME
#endif
#ifndef SpeedBarEnd
#define SpeedBarEnd     TAG_END)
#endif

#endif /* GADGETS_SPEEDBAR_H */
