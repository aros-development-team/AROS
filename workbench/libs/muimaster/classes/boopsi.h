/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_BOOPSI_H
#define _MUI_CLASSES_BOOPSI_H

/****************************************************************************/
/** Boopsi                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Boopsi[];
#else
#define MUIC_Boopsi "Boopsi.mui"
#endif

/* Attributes */

#define MUIA_Boopsi_Class                   0x80426999 /* V4  isg struct IClass *   */
#define MUIA_Boopsi_ClassID                 0x8042bfa3 /* V4  isg char *            */
#define MUIA_Boopsi_MaxHeight               0x8042757f /* V4  isg ULONG             */
#define MUIA_Boopsi_MaxWidth                0x8042bcb1 /* V4  isg ULONG             */
#define MUIA_Boopsi_MinHeight               0x80422c93 /* V4  isg ULONG             */
#define MUIA_Boopsi_MinWidth                0x80428fb2 /* V4  isg ULONG             */
#define MUIA_Boopsi_Object                  0x80420178 /* V4  ..g Object *          */
#define MUIA_Boopsi_Remember                0x8042f4bd /* V4  i.. ULONG             */
#define MUIA_Boopsi_Smart                   0x8042b8d7 /* V9  i.. BOOL              */
#define MUIA_Boopsi_TagDrawInfo             0x8042bae7 /* V4  isg ULONG             */
#define MUIA_Boopsi_TagScreen               0x8042bc71 /* V4  isg ULONG             */
#define MUIA_Boopsi_TagWindow               0x8042e11d /* V4  isg ULONG             */

#define MUIA_Boopsi_OnlyTrigger             0x8042e11e /* ZV1 PRIV (for notification) */

extern const struct __MUIBuiltinClass _MUI_Boopsi_desc;

#endif
