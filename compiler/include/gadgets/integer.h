/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/integer.h
*/

#ifndef GADGETS_INTEGER_H
#define GADGETS_INTEGER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define INTEGER_CLASSNAME       "gadgets/integer.gadget"
#define INTEGER_VERSION         44

#define INTEGER_Dummy           (TAG_USER + 0xB0000)

#define INTEGER_Number          (INTEGER_Dummy + 0x0001)
#define INTEGER_Minimum         (INTEGER_Dummy + 0x0002)
#define INTEGER_Maximum         (INTEGER_Dummy + 0x0003)
#define INTEGER_MaxChars        (INTEGER_Dummy + 0x0004)
#define INTEGER_Arrows          (INTEGER_Dummy + 0x0005)
#define INTEGER_IncDecAmount    (INTEGER_Dummy + 0x0006)
#define INTEGER_Justification   (INTEGER_Dummy + 0x0007)
#define INTEGER_WordWrap        (INTEGER_Dummy + 0x0008)

#define IntegerObject   NewObject(NULL, INTEGER_CLASSNAME
#define IntegerEnd      TAG_END)

#endif /* GADGETS_INTEGER_H */
