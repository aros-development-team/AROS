/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_STRING_H
#define _MUI_CLASSES_STRING_H

#define MUIC_String "String.mui"

/* String attributes */
#define MUIA_String_Accept           (TAG_USER|0x0042e3e1) /* V4  isg STRPTR        */
#define MUIA_String_Acknowledge      (TAG_USER|0x0042026c) /* V4  ..g STRPTR        */
#define MUIA_String_AdvanceOnCR      (TAG_USER|0x004226de) /* V11 isg BOOL          */
#define MUIA_String_AttachedList     (TAG_USER|0x00420fd2) /* V4  isg Object *      */
#define MUIA_String_BufferPos        (TAG_USER|0x00428b6c) /* V4  .sg LONG          */
#define MUIA_String_Contents         (TAG_USER|0x00428ffd) /* V4  isg STRPTR        */
#define MUIA_String_DisplayPos       (TAG_USER|0x0042ccbf) /* V4  .sg LONG          */
#define MUIA_String_EditHook         (TAG_USER|0x00424c33) /* V7  isg struct Hook * */
#define MUIA_String_Format           (TAG_USER|0x00427484) /* V4  i.g LONG          */
#define MUIA_String_Integer          (TAG_USER|0x00426e8a) /* V4  isg ULONG         */
#define MUIA_String_LonelyEditHook   (TAG_USER|0x00421569) /* V11 isg BOOL          */
#define MUIA_String_MaxLen           (TAG_USER|0x00424984) /* V4  i.g LONG          */
#define MUIA_String_Reject           (TAG_USER|0x0042179c) /* V4  isg STRPTR        */
#define MUIA_String_Secret           (TAG_USER|0x00428769) /* V4  i.g BOOL          */

enum {
    MUIV_String_Format_Left = 0,
    MUIV_String_Format_Center,
    MUIV_String_Format_Right,
};

extern const struct __MUIBuiltinClass _MUI_String_desc; /* PRIV */

#endif
