/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_COLORADJUST_H
#define _MUI_CLASSES_COLORADJUST_H

/*** Name *******************************************************************/
#define MUIC_Coloradjust        "Coloradjust.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Coloradjust        (MUIB_ZUNE | 0x00000700)

/*** Attributes *************************************************************/
#define MUIA_Coloradjust_Red 	(MUIB_MUI|0x00420eaa) /* isg ULONG   */
#define MUIA_Coloradjust_Green	(MUIB_MUI|0x004285ab) /* isg ULONG   */
#define MUIA_Coloradjust_Blue 	(MUIB_MUI|0x0042b8a3) /* isg ULONG   */
#define MUIA_Coloradjust_RGB 	(MUIB_MUI|0x0042f899) /* isg ULONG * */
#define MUIA_Coloradjust_ModeID (MUIB_MUI|0x0042ec59) /* isg ULONG   */


extern const struct __MUIBuiltinClass _MUI_Coloradjust_desc; /* PRIV */

#endif /* _MUI_CLASSES_COLORADJUST_H */
