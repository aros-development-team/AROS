/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/radiobutton.h
*/

#ifndef GADGETS_RADIOBUTTON_H
#define GADGETS_RADIOBUTTON_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define RADIOBUTTON_CLASSNAME   "gadgets/radiobutton.gadget"
#define RADIOBUTTON_VERSION     44

#define RADIOBUTTON_Dummy       (TAG_USER + 0xE0000)

#define RADIOBUTTON_Labels      (RADIOBUTTON_Dummy + 0x0001)
#define RADIOBUTTON_Selected    (RADIOBUTTON_Dummy + 0x0002)
#define RADIOBUTTON_Spacing     (RADIOBUTTON_Dummy + 0x0003)
#define RADIOBUTTON_Orientation (RADIOBUTTON_Dummy + 0x0004)
#define RADIOBUTTON_LabelPlace  (RADIOBUTTON_Dummy + 0x0005)

#define RadioButtonObject   NewObject(NULL, RADIOBUTTON_CLASSNAME
#define RadioButtonEnd      TAG_END)

#endif /* GADGETS_RADIOBUTTON_H */
