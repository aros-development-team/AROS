/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_COLORFIELD_H
#define _MUI_CLASSES_COLORFIELD_H

#define MUIC_Colorfield "Colorfield.mui"

#define MUIA_Colorfield_Pen 	(MUIB_MUI|0x0042713a) /* ..g  ULONG   */
#define MUIA_Colorfield_Red 	(MUIB_MUI|0x004279f6) /* isg  ULONG   */
#define MUIA_Colorfield_Green	(MUIB_MUI|0x00424466) /* isg  ULONG   */
#define MUIA_Colorfield_Blue 	(MUIB_MUI|0x0042d3b0) /* isg  ULONG   */
#define MUIA_Colorfield_RGB 	(MUIB_MUI|0x0042677a) /* isg  ULONG * */

extern const struct __MUIBuiltinClass _MUI_Colorfield_desc; /* PRIV */

#endif
