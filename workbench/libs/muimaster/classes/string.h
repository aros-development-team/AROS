/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_STRING_H
#define _MUI_CLASSES_STRING_H

struct MUI_StringData
{
    int dummy;
};

/****************************************************************************/
/** String                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_String[];
#else
#define MUIC_String "String.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_String_Accept                  0x8042e3e1 /* V4  isg STRPTR            */
#define MUIA_String_Acknowledge             0x8042026c /* V4  ..g STRPTR            */
#define MUIA_String_AdvanceOnCR             0x804226de /* V11 isg BOOL              */
#define MUIA_String_AttachedList            0x80420fd2 /* V4  isg Object *          */
#define MUIA_String_BufferPos               0x80428b6c /* V4  .sg LONG              */
#define MUIA_String_Contents                0x80428ffd /* V4  isg STRPTR            */
#define MUIA_String_DisplayPos              0x8042ccbf /* V4  .sg LONG              */
#define MUIA_String_EditHook                0x80424c33 /* V7  isg struct Hook *     */
#define MUIA_String_Format                  0x80427484 /* V4  i.g LONG              */
#define MUIA_String_Integer                 0x80426e8a /* V4  isg ULONG             */
#define MUIA_String_LonelyEditHook          0x80421569 /* V11 isg BOOL              */
#define MUIA_String_MaxLen                  0x80424984 /* V4  i.g LONG              */
#define MUIA_String_Reject                  0x8042179c /* V4  isg STRPTR            */
#define MUIA_String_Secret                  0x80428769 /* V4  i.g BOOL              */

#define MUIV_String_Format_Left 0
#define MUIV_String_Format_Center 1
#define MUIV_String_Format_Right 2

extern const struct __MUIBuiltinClass _MUI_String_desc;

#endif
