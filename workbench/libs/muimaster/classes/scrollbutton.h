/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_SCROLLBUTTON_H
#define _MUI_CLASSES_SCROLLBUTTON_H

#define MUIC_Scrollbutton "Scrollbutton.mui"

#define ScrollbuttonObject MUIOBJMACRO_START(MUIC_Scrollbutton)

#define MUIA_Scrollbutton_NewPosition       (MUIB_MUI|0x1820a000) /* ..G ULONG (packed two WORDs)*/
#define MUIA_Scrollbutton_Horiz             (MUIB_MUI|0x1820a001) /*     WORD */
#define MUIA_Scrollbutton_Vert              (MUIB_MUI|0x1820a002) /*     WORD */
#define MUIA_Scrollbutton_HorizProp         (MUIB_MUI|0x1820a003) /* ISG Object * */
#define MUIA_Scrollbutton_VertProp          (MUIB_MUI|0x1820a004) /* ISG Object * */

extern const struct __MUIBuiltinClass _MUI_Scrollbutton_desc; /* PRIV */

#endif  /* _MUI_CLASSES_SCROLLBUTTON_H */
