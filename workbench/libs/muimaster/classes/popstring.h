/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_POPSTRING_H
#define _MUI_CLASSES_POPSTRING_H

/****************************************************************************/
/** Popstring                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popstring[];
#else
#define MUIC_Popstring "Popstring.mui"
#endif

/* Methods */

#define MUIM_Popstring_Close                0x8042dc52 /* V7  */
#define MUIM_Popstring_Open                 0x804258ba /* V7  */
struct  MUIP_Popstring_Close                { ULONG MethodID; LONG result; };
struct  MUIP_Popstring_Open                 { ULONG MethodID; };

/* Attributes */

#define MUIA_Popstring_Button               0x8042d0b9 /* V7  i.g Object *          */
#define MUIA_Popstring_CloseHook            0x804256bf /* V7  isg struct Hook *     */
#define MUIA_Popstring_OpenHook             0x80429d00 /* V7  isg struct Hook *     */
#define MUIA_Popstring_String               0x804239ea /* V7  i.g Object *          */
#define MUIA_Popstring_Toggle               0x80422b7a /* V7  isg BOOL              */


extern const struct __MUIBuiltinClass _MUI_Popstring_desc;

#endif
