/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/radiobutton.h
*/

#ifndef GADGETS_RADIOBUTTON_H
#define GADGETS_RADIOBUTTON_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define RADIOBUTTON_CLASSNAME   "radiobutton.gadget"
#define RADIOBUTTON_VERSION     44

/* Node attributes for radio button entries */
#define RBNA_Dummy              (TAG_USER + 0x020000)
#define RBNA_UserData           (RBNA_Dummy + 1)  /* Per-node user data (APTR) */
#define RBNA_Labels             (RBNA_Dummy + 2)  /* Node label string (STRPTR) */

#define RADIOBUTTON_Dummy       (REACTION_Dummy + 0x14000)

#define RADIOBUTTON_Labels      (RADIOBUTTON_Dummy + 1)  /* Label list (struct List *) */
#define RADIOBUTTON_Strings     (RADIOBUTTON_Dummy + 2)  /* Reserved - unsupported */
#define RADIOBUTTON_Spacing     (RADIOBUTTON_Dummy + 3)  /* Space between buttons (WORD) */
#define RADIOBUTTON_Selected    (RADIOBUTTON_Dummy + 4)  /* Active selection index (WORD) */
#define RADIOBUTTON_LabelPlace  (RADIOBUTTON_Dummy + 5)  /* Label placement (WORD) */

#ifndef RadioButtonObject
#define RadioButtonObject   NewObject(NULL, RADIOBUTTON_CLASSNAME
#endif
#ifndef RadioButtonEnd
#define RadioButtonEnd      TAG_END)
#endif

#endif /* GADGETS_RADIOBUTTON_H */
