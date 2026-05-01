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

#define TEXTFIELD_CLASSNAME "textfield.gadget"
#define TEXTFIELD_VERSION   44

#define TEXTFIELD_Dummy     (TAG_USER + 0x140000)

#define TEXTFIELD_Text          (TEXTFIELD_Dummy + 0x0001) /* Text content */
#define TEXTFIELD_MaxChars      (TEXTFIELD_Dummy + 0x0002) /* Max characters */
#define TEXTFIELD_ReadOnly      (TEXTFIELD_Dummy + 0x0003) /* Non-editable */
#define TEXTFIELD_Partial       (TEXTFIELD_Dummy + 0x0004) /* Partial text update */
#define TEXTFIELD_WordWrap      (TEXTFIELD_Dummy + 0x0005) /* Word wrap mode */
#define TEXTFIELD_VScroller     (TEXTFIELD_Dummy + 0x0006) /* Vertical scrollbar */
#define TEXTFIELD_HScroller     (TEXTFIELD_Dummy + 0x0007) /* Horizontal scrollbar */
#define TEXTFIELD_CursorPos     (TEXTFIELD_Dummy + 0x0008) /* Cursor offset */
#define TEXTFIELD_Lines         (TEXTFIELD_Dummy + 0x0009) /* Total line count */
#define TEXTFIELD_Top           (TEXTFIELD_Dummy + 0x000A) /* Top visible line */
#define TEXTFIELD_Blinkrate     (TEXTFIELD_Dummy + 0x000B) /* Cursor blink rate */
#define TEXTFIELD_NoGhost       (TEXTFIELD_Dummy + 0x000C) /* No ghosting when disabled */
#define TEXTFIELD_TabSpaces     (TEXTFIELD_Dummy + 0x000D) /* Spaces per tab */
#define TEXTFIELD_Columns       (TEXTFIELD_Dummy + 0x000E) /* Visible columns */

#ifndef TextFieldObject
#define TextFieldObject NewObject(NULL, TEXTFIELD_CLASSNAME
#endif
#ifndef TextFieldEnd
#define TextFieldEnd    TAG_END)
#endif

#endif /* GADGETS_TEXTFIELD_H */
