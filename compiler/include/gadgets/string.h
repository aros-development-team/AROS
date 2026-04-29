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

#define STRINGA_Dummy       (REACTION_Dummy + 0x0055000)

#define STRINGA_MaxChars        (STRINGA_Dummy + 0x0001) /* Buffer size limit */
#define STRINGA_Buffer          (STRINGA_Dummy + 0x0002) /* String buffer */
#define STRINGA_BufferPos       (STRINGA_Dummy + 0x0003) /* Cursor position */
#define STRINGA_DispPos         (STRINGA_Dummy + 0x0004) /* Display scroll offset */
#define STRINGA_UndoBuffer      (STRINGA_Dummy + 0x0005) /* Undo buffer */
#define STRINGA_WorkBuffer      (STRINGA_Dummy + 0x0006) /* Work buffer */
#define STRINGA_Justification   (STRINGA_Dummy + 0x0007) /* Text alignment */
#define STRINGA_TextVal         (STRINGA_Dummy + 0x0008) /* String value */
#define STRINGA_LongVal         (STRINGA_Dummy + 0x0009) /* Integer value */
#define STRINGA_EditHook        (STRINGA_Dummy + 0x000A) /* Custom edit hook */
#define STRINGA_ReplaceMode     (STRINGA_Dummy + 0x000B) /* Overwrite mode */

#ifndef StringObject
#define StringObject    NewObject(NULL, STRING_CLASSNAME
#endif
#ifndef StringEnd
#define StringEnd       TAG_END)
#endif

#endif /* GADGETS_STRING_H */
