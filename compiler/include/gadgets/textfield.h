/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/textfield.h
*/

#ifndef GADGETS_TEXTFIELD_H
#define GADGETS_TEXTFIELD_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define TEXTFIELD_CLASSNAME "gadgets/textfield.gadget"
#define TEXTFIELD_VERSION   44

#define TEXTFIELD_Dummy     (TAG_USER + 0x140000)

#define TEXTFIELD_Text          (TEXTFIELD_Dummy + 0x0001)
#define TEXTFIELD_MaxChars      (TEXTFIELD_Dummy + 0x0002)
#define TEXTFIELD_ReadOnly      (TEXTFIELD_Dummy + 0x0003)
#define TEXTFIELD_Partial       (TEXTFIELD_Dummy + 0x0004)
#define TEXTFIELD_WordWrap      (TEXTFIELD_Dummy + 0x0005)
#define TEXTFIELD_VScroller     (TEXTFIELD_Dummy + 0x0006)
#define TEXTFIELD_HScroller     (TEXTFIELD_Dummy + 0x0007)
#define TEXTFIELD_CursorPos     (TEXTFIELD_Dummy + 0x0008)
#define TEXTFIELD_Lines         (TEXTFIELD_Dummy + 0x0009)
#define TEXTFIELD_Top           (TEXTFIELD_Dummy + 0x000A)
#define TEXTFIELD_Blinkrate     (TEXTFIELD_Dummy + 0x000B)
#define TEXTFIELD_NoGhost       (TEXTFIELD_Dummy + 0x000C)
#define TEXTFIELD_TabSpaces     (TEXTFIELD_Dummy + 0x000D)
#define TEXTFIELD_Columns       (TEXTFIELD_Dummy + 0x000E)

#define TextFieldObject NewObject(NULL, TEXTFIELD_CLASSNAME
#define TextFieldEnd    TAG_END)

#endif /* GADGETS_TEXTFIELD_H */
