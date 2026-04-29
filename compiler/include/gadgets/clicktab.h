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

#define CLICKTAB_Dummy      (REACTION_Dummy + 0x27000)

#define CLICKTAB_Labels         (CLICKTAB_Dummy + 0x0001) /* List of ClickTabNodes */
#define CLICKTAB_Current        (CLICKTAB_Dummy + 0x0002) /* Active tab index */
#define CLICKTAB_NumTabs        (CLICKTAB_Dummy + 0x0003) /* Tab count */
#define CLICKTAB_PageObject     (CLICKTAB_Dummy + 0x0004) /* Linked page object */
#define CLICKTAB_PageGroup      (CLICKTAB_Dummy + 0x0005) /* Linked page group */
#define CLICKTAB_CloseImage     (CLICKTAB_Dummy + 0x0006) /* Close button image */
#define CLICKTAB_FlagImage      (CLICKTAB_Dummy + 0x0007) /* Flag indicator image */
#define CLICKTAB_LabelTruncate  (CLICKTAB_Dummy + 0x0008) /* Truncate long labels */
#define CLICKTAB_CurrentNode    (CLICKTAB_Dummy + 0x0009) /* Clicked node pointer */
#define CLICKTAB_BackgroundPen  (CLICKTAB_Dummy + 0x000A) /* Background pen */

/* ClickTabNode tags */
#define TNA_Dummy           (TAG_USER + 0x010000)
#define TNA_Text            (TNA_Dummy + 0x0001) /* Tab label */
#define TNA_Number          (TNA_Dummy + 0x0002) /* Tab index */
#define TNA_Disabled        (TNA_Dummy + 0x0003) /* Disabled state */
#define TNA_Image           (TNA_Dummy + 0x0004) /* Tab image */
#define TNA_SelImage        (TNA_Dummy + 0x0005) /* Selected tab image */
#define TNA_UserData        (TNA_Dummy + 0x0006) /* User data pointer */
#define TNA_Closable        (TNA_Dummy + 0x0007) /* Tab is closeable */
#define TNA_Flagged         (TNA_Dummy + 0x0008) /* Tab is flagged */
#define TNA_CloseGadget     (TNA_Dummy + 0x0009) /* Close gadget object */
#define TNA_HintInfo        (TNA_Dummy + 0x000A) /* Tooltip text */

#ifndef ClickTabObject
#define ClickTabObject  NewObject(NULL, CLICKTAB_CLASSNAME
#endif
#ifndef ClickTabEnd
#define ClickTabEnd     TAG_END)
#endif

#endif /* GADGETS_CLICKTAB_H */
