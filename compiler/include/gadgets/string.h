/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/string.h (ReAction version)
*/

#ifndef GADGETS_STRING_H
#define GADGETS_STRING_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define STRING_CLASSNAME    "gadgets/string.gadget"
#define STRING_VERSION      44

#define STRINGA_Dummy       (TAG_USER + 0x130000)

#define STRINGA_MaxChars        (STRINGA_Dummy + 0x0001)
#define STRINGA_Buffer          (STRINGA_Dummy + 0x0002)
#define STRINGA_BufferPos       (STRINGA_Dummy + 0x0003)
#define STRINGA_DispPos         (STRINGA_Dummy + 0x0004)
#define STRINGA_UndoBuffer      (STRINGA_Dummy + 0x0005)
#define STRINGA_WorkBuffer      (STRINGA_Dummy + 0x0006)
#define STRINGA_Justification   (STRINGA_Dummy + 0x0007)
#define STRINGA_TextVal         (STRINGA_Dummy + 0x0008)
#define STRINGA_LongVal         (STRINGA_Dummy + 0x0009)
#define STRINGA_EditHook        (STRINGA_Dummy + 0x000A)
#define STRINGA_ReplaceMode     (STRINGA_Dummy + 0x000B)

#define StringObject    NewObject(NULL, STRING_CLASSNAME
#define StringEnd       TAG_END)

#endif /* GADGETS_STRING_H */
