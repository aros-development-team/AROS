#ifndef _MUI_CLASSES_STRING_H
#define _MUI_CLASSES_STRING_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_String                  "String.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_String                  (MUIB_ZUNE | 0x00003400)

/*** Attributes *************************************************************/
#define MUIA_String_Accept           (MUIB_MUI|0x0042e3e1) /* V4  isg STRPTR        */
#define MUIA_String_Acknowledge      (MUIB_MUI|0x0042026c) /* V4  ..g STRPTR        */
#define MUIA_String_AdvanceOnCR      (MUIB_MUI|0x004226de) /* V11 isg BOOL          */
#define MUIA_String_AttachedList     (MUIB_MUI|0x00420fd2) /* V4  isg Object *      */
#define MUIA_String_BufferPos        (MUIB_MUI|0x00428b6c) /* V4  .sg LONG          */
#define MUIA_String_Contents         (MUIB_MUI|0x00428ffd) /* V4  isg STRPTR        */
#define MUIA_String_DisplayPos       (MUIB_MUI|0x0042ccbf) /* V4  .sg LONG          */
#define MUIA_String_EditHook         (MUIB_MUI|0x00424c33) /* V7  isg struct Hook * */
#define MUIA_String_Format           (MUIB_MUI|0x00427484) /* V4  i.g LONG          */
#define MUIA_String_Integer          (MUIB_MUI|0x00426e8a) /* V4  isg ULONG         */
#define MUIA_String_LonelyEditHook   (MUIB_MUI|0x00421569) /* V11 isg BOOL          */
#define MUIA_String_MaxLen           (MUIB_MUI|0x00424984) /* V4  i.g LONG          */
#define MUIA_String_Reject           (MUIB_MUI|0x0042179c) /* V4  isg STRPTR        */
#define MUIA_String_Secret           (MUIB_MUI|0x00428769) /* V4  i.g BOOL          */

enum {
    MUIV_String_Format_Left = 0,
    MUIV_String_Format_Center,
    MUIV_String_Format_Right,
};

/* Extended features taken over from Alan Odgaard's BetterString MCC.
   Attribute and method IDs match those of BetterString class. */

#define MUIA_String_Columns         	    0xad001005
#define MUIA_String_NoInput         	    0xad001007
#define MUIA_String_SelectSize      	    0xad001001
#define MUIA_String_StayActive      	    0xad001003
#define MUIA_String_KeyUpFocus      	    0xad001008
#define MUIA_String_KeyDownFocus    	    0xad001009

#define MUIM_String_ClearSelected   	    0xad001004
#define MUIM_String_FileNameStart   	    0xad001006
#define MUIM_String_Insert          	    0xad001002

#define MUIV_String_Insert_StartOfString    0x00000000
#define MUIV_String_Insert_EndOfString      0xfffffffe
#define MUIV_String_Insert_BufferPos        0xffffffff
#define MUIV_String_BufferPos_End           0xffffffff

#define MUIR_String_FileNameStart_Volume    0xffffffff

struct MUIP_String_ClearSelected {ULONG MethodID;};
struct MUIP_String_FileNameStart {ULONG MethodID; STRPTR buffer; LONG pos;};
struct MUIP_String_Insert        {ULONG MethodID; STRPTR text; LONG pos;};

extern const struct __MUIBuiltinClass _MUI_String_desc; /* PRIV */

#endif /* _MUI_CLASSES_STRING_H */
