/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/chooser.h
*/

#ifndef GADGETS_CHOOSER_H
#define GADGETS_CHOOSER_H

#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif

#define CHOOSER_CLASSNAME   "chooser.gadget"
#define CHOOSER_VERSION     45

/* Predefined minimum dimensions for safe operation */
#define CHOOSER_MinWidth    36
#define CHOOSER_MinHeight   10

/*****************************************************************************/

/* Chooser node attributes */

#define CNA_Dummy           (TAG_USER+0x5001500)

#define CNA_Text            (CNA_Dummy+1)   /* (STRPTR) Node label text */
#define CNA_Image           (CNA_Dummy+2)   /* (struct Image *) Normal image */
#define CNA_SelImage        (CNA_Dummy+3)   /* (struct Image *) Selected image */
#define CNA_UserData        (CNA_Dummy+4)   /* (APTR) User data */
#define CNA_Separator       (CNA_Dummy+5)   /* (BOOL) Render separator bar */
#define CNA_Disabled        (CNA_Dummy+6)   /* (BOOL) Disabled entry */
#define CNA_BGPen           (CNA_Dummy+7)   /* (WORD) Background pen (unimplemented) */
#define CNA_FGPen           (CNA_Dummy+8)   /* (WORD) Foreground pen (unimplemented) */
#define CNA_ReadOnly        (CNA_Dummy+9)   /* (BOOL) Non-selectable entry */

/*****************************************************************************/

/* Additional attributes defined by the Chooser class */

#define CHOOSER_Dummy       (REACTION_Dummy + 0x0001000)

#define CHOOSER_PopUp           (CHOOSER_Dummy+1)   /* (BOOL) Popup menu mode */
#define CHOOSER_DropDown        (CHOOSER_Dummy+2)   /* (BOOL) Dropdown menu mode */
#define CHOOSER_Title           (CHOOSER_Dummy+3)   /* (STRPTR) Dropdown title */
#define CHOOSER_Labels          (CHOOSER_Dummy+4)   /* (struct List *) Label list */
#define CHOOSER_Active          (CHOOSER_Dummy+5)   /* (WORD) Active label index */
#define CHOOSER_Selected        (CHOOSER_Active)    /* Alias for CHOOSER_Active */
#define CHOOSER_Width           (CHOOSER_Dummy+6)   /* (WORD) Popup menu width */
#define CHOOSER_AutoFit         (CHOOSER_Dummy+7)   /* (BOOL) Auto-fit to labels */
#define CHOOSER_MaxLabels       (CHOOSER_Dummy+9)   /* (WORD) Max visible labels */
#define CHOOSER_Offset          (CHOOSER_Dummy+10)  /* (WORD) Value offset for notifications */
#define CHOOSER_Hidden          (CHOOSER_Dummy+11)  /* (BOOL) Hidden chooser mode */
#define CHOOSER_LabelArray      (CHOOSER_Dummy+12)  /* (STRPTR *) Null-terminated string array */
#define CHOOSER_Justification   (CHOOSER_Dummy+13)  /* (WORD) Label alignment */

/* Justification modes for CHOOSER_Justification */
#define CHJ_LEFT    0
#define CHJ_CENTER  1
#define CHJ_RIGHT   2

/*****************************************************************************/

#ifndef ChooserObject
#define ChooserObject   NewObject(NULL, CHOOSER_CLASSNAME
#endif
#ifndef ChooserEnd
#define ChooserEnd      TAG_END)
#endif

#endif /* GADGETS_CHOOSER_H */
