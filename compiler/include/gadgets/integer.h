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

#define INTEGER_Number          (INTEGER_Dummy + 1) /* (LONG) Gadget value, default 0 */
#define INTEGER_MaxChars        (INTEGER_Dummy + 2) /* (WORD) Max digits incl. sign, default 10 */
#define INTEGER_Minimum         (INTEGER_Dummy + 3) /* (LONG) Minimum allowed value */
#define INTEGER_Maximum         (INTEGER_Dummy + 4) /* (LONG) Maximum allowed value */
#define INTEGER_Arrows          (INTEGER_Dummy + 5) /* (BOOL) Show arrows, default TRUE */
#define INTEGER_MinVisible      (INTEGER_Dummy + 6) /* (WORD) Min visible digits (V41) */
#define INTEGER_SkipVal         (INTEGER_Dummy + 7) /* (LONG) Arrow step amount (V45) */

#ifndef IntegerObject
#define IntegerObject   NewObject(NULL, INTEGER_CLASSNAME
#endif
#ifndef IntegerEnd
#define IntegerEnd      TAG_END)
#endif

#endif /* GADGETS_INTEGER_H */
