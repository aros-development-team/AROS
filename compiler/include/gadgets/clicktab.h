/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/clicktab.h
*/

#ifndef GADGETS_CLICKTAB_H
#define GADGETS_CLICKTAB_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define CLICKTAB_CLASSNAME  "clicktab.gadget"
#define CLICKTAB_VERSION    44

/* ClickTabNode attributes */
#define TNA_Dummy           (TAG_USER + 0x010000)
#define TNA_UserData        (TNA_Dummy + 1)  /* (APTR) User data pointer */
#define TNA_Enabled         (TNA_Dummy + 2)  /* Obsolete, not implemented */
#define TNA_Spacing         (TNA_Dummy + 3)  /* Obsolete */
#define TNA_Highlight       (TNA_Dummy + 4)  /* Obsolete */
#define TNA_Image           (TNA_Dummy + 5)  /* (struct Image *) Render image */
#define TNA_SelImage        (TNA_Dummy + 6)  /* (struct Image *) Selected image */
#define TNA_Text            (TNA_Dummy + 7)  /* (STRPTR) Tab label string */
#define TNA_Number          (TNA_Dummy + 8)  /* (WORD) Numeric tab ID */
#define TNA_TextPen         (TNA_Dummy + 9)  /* (WORD) Pen for tab text */
#define TNA_Disabled        (TNA_Dummy + 10) /* (BOOL) Tab disabled state (V42) */

/* ClickTab gadget attributes */
#define CLICKTAB_Dummy              (REACTION_Dummy + 0x27000)
#define CLICKTAB_Labels             (CLICKTAB_Dummy + 1) /* (struct List *) Tab node list */
#define CLICKTAB_Current            (CLICKTAB_Dummy + 2) /* (WORD) Selected tab index */
#define CLICKTAB_CurrentNode        (CLICKTAB_Dummy + 3) /* (struct Node *) Selected tab node */
#define CLICKTAB_Orientation        (CLICKTAB_Dummy + 4) /* (WORD) Layout orientation */
#define CLICKTAB_PageGroup          (CLICKTAB_Dummy + 5) /* (Object *) Embedded page object (V42) */
#define CLICKTAB_PageGroupBackFill  (CLICKTAB_Dummy + 6) /* (Object *) Page + backfill ptr (V42) */

/* CLICKTAB_Orientation modes */
#define CTORIENT_HORIZ      0
#define CTORIENT_VERT       1
#define CTORIENT_HORIZFLIP  2
#define CTORIENT_VERTFLIP   3

#ifndef ClickTabObject
#define ClickTabObject  NewObject(NULL, CLICKTAB_CLASSNAME
#endif
#ifndef ClickTabEnd
#define ClickTabEnd     TAG_END)
#endif

#endif /* GADGETS_CLICKTAB_H */
