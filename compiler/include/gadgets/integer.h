/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/integer.h
*/

#ifndef GADGETS_INTEGER_H
#define GADGETS_INTEGER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define INTEGER_CLASSNAME       "integer.gadget"
#define INTEGER_VERSION         44

#define INTEGER_Dummy           (REACTION_Dummy + 0x0002000)

#define INTEGER_Number          (INTEGER_Dummy + 0x0001) /* Current value */
#define INTEGER_Minimum         (INTEGER_Dummy + 0x0002) /* Minimum allowed */
#define INTEGER_Maximum         (INTEGER_Dummy + 0x0003) /* Maximum allowed */
#define INTEGER_MaxChars        (INTEGER_Dummy + 0x0004) /* Max input characters */
#define INTEGER_Arrows          (INTEGER_Dummy + 0x0005) /* Show inc/dec arrows */
#define INTEGER_IncDecAmount    (INTEGER_Dummy + 0x0006) /* Arrow step amount */
#define INTEGER_Justification   (INTEGER_Dummy + 0x0007) /* Text alignment */
#define INTEGER_WordWrap        (INTEGER_Dummy + 0x0008) /* Wrap text */

#ifndef IntegerObject
#define IntegerObject   NewObject(NULL, INTEGER_CLASSNAME
#endif
#ifndef IntegerEnd
#define IntegerEnd      TAG_END)
#endif

#endif /* GADGETS_INTEGER_H */
