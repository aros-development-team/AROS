/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/chooser.h
*/

#ifndef GADGETS_CHOOSER_H
#define GADGETS_CHOOSER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * chooser.gadget - ClassAct/ReAction compatible popup/dropdown chooser
 *
 * Superclass: gadgetclass
 * Include:    <gadgets/chooser.h>
 */

#define CHOOSER_CLASSNAME   "gadgets/chooser.gadget"
#define CHOOSER_VERSION     44

/* Tag base */
#define CHOOSER_Dummy       (TAG_USER + 0x40000)

/* Attributes */
/* (I..) Exec list of ChooserNode items */
#define CHOOSER_Labels          (CHOOSER_Dummy + 0x0001)
/* (ISG) Currently selected item (0-based) */
#define CHOOSER_Selected        (CHOOSER_Dummy + 0x0002)
/* (I..) Maximum number of visible items */
#define CHOOSER_MaxLabels       (CHOOSER_Dummy + 0x0003)
/* (I..) Render as dropdown */
#define CHOOSER_DropDown        (CHOOSER_Dummy + 0x0004)
/* (I..) Auto-fit width */
#define CHOOSER_AutoFit         (CHOOSER_Dummy + 0x0005)
/* (ISG) Title string */
#define CHOOSER_Title           (CHOOSER_Dummy + 0x0006)
/* (I..) Read only mode */
#define CHOOSER_ReadOnly        (CHOOSER_Dummy + 0x0007)
/* (I..) Label array (STRPTR *) */
#define CHOOSER_LabelArray      (CHOOSER_Dummy + 0x0008)
/* (I..) Popup mode */
#define CHOOSER_PopUp           (CHOOSER_Dummy + 0x0009)
/* (I..) Hide */
#define CHOOSER_Hidden          (CHOOSER_Dummy + 0x000A)

/* ChooserNode attributes (for AllocChooserNodeA) */
#define CNA_Dummy           (TAG_USER + 0x40100)
#define CNA_Text            (CNA_Dummy + 0x0001)
#define CNA_Image           (CNA_Dummy + 0x0002)
#define CNA_Disabled        (CNA_Dummy + 0x0003)
#define CNA_Selected        (CNA_Dummy + 0x0004)
#define CNA_Separator       (CNA_Dummy + 0x0005)
#define CNA_UserData        (CNA_Dummy + 0x0006)
#define CNA_ReadOnly        (CNA_Dummy + 0x0007)

/* Object creation macros */
#define ChooserObject   NewObject(NULL, CHOOSER_CLASSNAME
#define ChooserEnd      TAG_END)

#endif /* GADGETS_CHOOSER_H */
