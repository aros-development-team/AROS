/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_SLIDER_H
#define _MUI_CLASSES_SLIDER_H

#define MUIC_Slider "Slider.mui"

/* Slider attributes */
#define MUIA_Slider_Horiz    (MUIB_MUI|0x0042fad1) /* V11 isg BOOL */
#define MUIA_Slider_Quiet    (MUIB_MUI|0x00420b26) /* V6  i.. BOOL */

#ifdef MUI_OBSOLETE
#define MUIA_Slider_Level    (MUIB_MUI|0x0042ae3a) /* V4  isg LONG */
#define MUIA_Slider_Max      (MUIB_MUI|0x0042d78a) /* V4  isg LONG */
#define MUIA_Slider_Min      (MUIB_MUI|0x0042e404) /* V4  isg LONG */
#define MUIA_Slider_Reverse  (MUIB_MUI|0x0042f2a0) /* V4  isg BOOL */
#endif /* MUI_OBSOLETE */

extern const struct __MUIBuiltinClass _MUI_Slider_desc; /* PRIV */

#endif
