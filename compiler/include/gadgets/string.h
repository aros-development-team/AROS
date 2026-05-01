/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/string.h (ReAction version)
*/

#ifndef GADGETS_STRING_H
#define GADGETS_STRING_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define STRING_CLASSNAME    "string.gadget"
#define STRING_VERSION      44

/*
 * string.gadget accepts the same tags as the ROM strgclass,
 * plus the following Reaction-specific extensions.
 */

#define STRINGA_MinVisible      (REACTION_Dummy + 0x0055000)
    /* (UWORD) Minimum visible character count used for domain sizing. */

#define STRINGA_HookType        (REACTION_Dummy + 0x0055001)
    /* (UWORD) Select a built-in editing hook. */

/* v45+ tags */
#define STRINGA_GetBlockPos     (REACTION_Dummy + 0x0055010)
    /* (ULONG) Marked block positions: high word = start, low word = end. */

#define STRINGA_Mark            (REACTION_Dummy + 0x0055011)
    /* (ULONG) Set marked block: high word = start, low word = end. */

#define STRINGA_AllowMarking    (REACTION_Dummy + 0x0055012)
    /* (BOOL) Enable or disable text marking. Default TRUE. */

/* Built-in hook types for STRINGA_HookType */
#define SHK_CUSTOM      0
#define SHK_PASSWORD    1
#define SHK_IPADDRESS   2
#define SHK_FLOAT       3
#define SHK_HEXIDECIMAL 4
#define SHK_TELEPHONE   5
#define SHK_POSTALCODE  6
#define SHK_AMOUNT      7
#define SHK_UPPERCASE   8
#define SHK_HOTKEY      9   /* v45+ */

#define SHK_HEXADECIMAL SHK_HEXIDECIMAL

#ifndef StringObject
#define StringObject    NewObject(NULL, STRING_CLASSNAME
#endif
#ifndef StringEnd
#define StringEnd       TAG_END)
#endif

#endif /* GADGETS_STRING_H */
