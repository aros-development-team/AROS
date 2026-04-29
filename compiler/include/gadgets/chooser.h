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
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define CHOOSER_CLASSNAME   "chooser.gadget"
#define CHOOSER_VERSION     44

#define CHOOSER_Dummy       (REACTION_Dummy + 0x0001000)

#define CHOOSER_Labels          (CHOOSER_Dummy + 0x0001) /* List of ChooserNodes */
#define CHOOSER_Selected        (CHOOSER_Dummy + 0x0002) /* Active item index */
#define CHOOSER_MaxLabels       (CHOOSER_Dummy + 0x0003) /* Max visible entries */
#define CHOOSER_DropDown        (CHOOSER_Dummy + 0x0004) /* Dropdown style */
#define CHOOSER_AutoFit         (CHOOSER_Dummy + 0x0005) /* Fit width to content */
#define CHOOSER_Title           (CHOOSER_Dummy + 0x0006) /* Title text */
#define CHOOSER_ReadOnly        (CHOOSER_Dummy + 0x0007) /* Non-editable */
#define CHOOSER_LabelArray      (CHOOSER_Dummy + 0x0008) /* STRPTR array of labels */
#define CHOOSER_PopUp           (CHOOSER_Dummy + 0x0009) /* Popup style */
#define CHOOSER_Hidden          (CHOOSER_Dummy + 0x000A) /* Hidden state */

/* ChooserNode tags */
#define CNA_Dummy           (TAG_USER + 0x5001500)
#define CNA_Text            (CNA_Dummy + 0x0001) /* Node label */
#define CNA_Image           (CNA_Dummy + 0x0002) /* Node image */
#define CNA_Disabled        (CNA_Dummy + 0x0003) /* Disabled state */
#define CNA_Selected        (CNA_Dummy + 0x0004) /* Selected state */
#define CNA_Separator       (CNA_Dummy + 0x0005) /* Separator line */
#define CNA_UserData        (CNA_Dummy + 0x0006) /* User data pointer */
#define CNA_ReadOnly        (CNA_Dummy + 0x0007) /* Non-selectable */

#ifndef ChooserObject
#define ChooserObject   NewObject(NULL, CHOOSER_CLASSNAME
#endif
#ifndef ChooserEnd
#define ChooserEnd      TAG_END)
#endif

#endif /* GADGETS_CHOOSER_H */
