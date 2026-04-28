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
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * clicktab.gadget - ClassAct/ReAction compatible tabbed gadget
 *
 * Superclass: gadgetclass
 * Include:    <gadgets/clicktab.h>
 */

#define CLICKTAB_CLASSNAME  "gadgets/clicktab.gadget"
#define CLICKTAB_VERSION    44

/* Tag base */
#define CLICKTAB_Dummy      (TAG_USER + 0x50000)

/* Attributes */
/* (I..) Exec list of ClickTabNode items */
#define CLICKTAB_Labels         (CLICKTAB_Dummy + 0x0001)
/* (ISG) Currently selected tab (0-based) */
#define CLICKTAB_Current        (CLICKTAB_Dummy + 0x0002)
/* (I..) Total number of tabs */
#define CLICKTAB_NumTabs        (CLICKTAB_Dummy + 0x0003)
/* (I..) Page object (for automatic page switching) */
#define CLICKTAB_PageObject     (CLICKTAB_Dummy + 0x0004)
/* (I..) Page group */
#define CLICKTAB_PageGroup      (CLICKTAB_Dummy + 0x0005)
/* (I..) CloseImage - for closeable tabs */
#define CLICKTAB_CloseImage     (CLICKTAB_Dummy + 0x0006)
/* (I..) Flag image */
#define CLICKTAB_FlagImage      (CLICKTAB_Dummy + 0x0007)
/* (I..) Label truncation */
#define CLICKTAB_LabelTruncate  (CLICKTAB_Dummy + 0x0008)
/* (I..) Current clicked tab */
#define CLICKTAB_CurrentNode    (CLICKTAB_Dummy + 0x0009)
/* (I..) Tab background pen */
#define CLICKTAB_BackgroundPen  (CLICKTAB_Dummy + 0x000A)

/* ClickTabNode attributes (for AllocClickTabNodeA) */
#define TNA_Dummy           (TAG_USER + 0x50100)
#define TNA_Text            (TNA_Dummy + 0x0001)
#define TNA_Number          (TNA_Dummy + 0x0002)
#define TNA_Disabled        (TNA_Dummy + 0x0003)
#define TNA_Image           (TNA_Dummy + 0x0004)
#define TNA_SelImage        (TNA_Dummy + 0x0005)
#define TNA_UserData        (TNA_Dummy + 0x0006)
#define TNA_Closable        (TNA_Dummy + 0x0007)
#define TNA_Flagged         (TNA_Dummy + 0x0008)
#define TNA_CloseGadget     (TNA_Dummy + 0x0009)
#define TNA_HintInfo        (TNA_Dummy + 0x000A)

/* Object creation macros */
#define ClickTabObject  NewObject(NULL, CLICKTAB_CLASSNAME
#define ClickTabEnd     TAG_END)

#endif /* GADGETS_CLICKTAB_H */
