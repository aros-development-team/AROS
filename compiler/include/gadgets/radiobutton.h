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

#define RADIOBUTTON_Dummy       (REACTION_Dummy + 0x14000)

#define RADIOBUTTON_Labels      (RADIOBUTTON_Dummy + 0x0001) /* Label list */
#define RADIOBUTTON_Selected    (RADIOBUTTON_Dummy + 0x0002) /* Active choice index */
#define RADIOBUTTON_Spacing     (RADIOBUTTON_Dummy + 0x0003) /* Space between items */
#define RADIOBUTTON_Orientation (RADIOBUTTON_Dummy + 0x0004) /* Horiz or vert layout */
#define RADIOBUTTON_LabelPlace  (RADIOBUTTON_Dummy + 0x0005) /* Label position */

/* Node attributes for AllocRadioButtonNode() */
#define RBNA_Dummy              (TAG_USER + 0x020000)
#define RBNA_UserData           (RBNA_Dummy + 1) /* Per-node user data */
#define RBNA_Labels             (RBNA_Dummy + 2) /* Node label string */
#define RBNA_Disabled           (RBNA_Dummy + 3) /* Node disabled state */

#ifndef RadioButtonObject
#define RadioButtonObject   NewObject(NULL, RADIOBUTTON_CLASSNAME
#endif
#ifndef RadioButtonEnd
#define RadioButtonEnd      TAG_END)
#endif

#endif /* GADGETS_RADIOBUTTON_H */
