/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_COLORADJUST_H
#define _MUI_CLASSES_COLORADJUST_H

#define MUIC_Coloradjust "Coloradjust.mui"

#define MUIA_Coloradjust_Red 	(TAG_USER|0x00420eaa) /* isg ULONG   */
#define MUIA_Coloradjust_Green	(TAG_USER|0x004285ab) /* isg ULONG   */
#define MUIA_Coloradjust_Blue 	(TAG_USER|0x0042b8a3) /* isg ULONG   */
#define MUIA_Coloradjust_RGB 	(TAG_USER|0x0042f899) /* isg ULONG * */
#define MUIA_Coloradjust_ModeID (TAG_USER|0x0042ec59) /* isg ULONG   */

extern const struct __MUIBuiltinClass _MUI_Coloradjust_desc; /* PRIV */

#endif
