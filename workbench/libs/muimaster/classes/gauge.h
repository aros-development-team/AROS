#ifndef _MUI_CLASSES_GAUGE_H
#define _MUI_CLASSES_GAUGE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Gauge          "Gauge.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Gauge          (MUIB_ZUNE | 0x00000f00)

/*** Attributes *************************************************************/
#define MUIA_Gauge_Current  (MUIB_MUI|0x0042f0dd) /* MUI: V4  isg LONG    */
#define MUIA_Gauge_Divide   (MUIB_MUI|0x0042d8df) /* MUI: V4  isg LONG    */
#define MUIA_Gauge_Horiz    (MUIB_MUI|0x004232dd) /* MUI: V4  i.. BOOL    */
#define MUIA_Gauge_InfoText (MUIB_MUI|0x0042bf15) /* MUI: V7  isg STRPTR  */
#define MUIA_Gauge_Max      (MUIB_MUI|0x0042bcdb) /* MUI: V4  isg LONG    */

#define MUIA_Gauge_DupInfoText (MUIB_Gauge | 0x00000000) /* ZUNE: V1  i.. BOOL - defaults to FALSE */


extern const struct __MUIBuiltinClass _MUI_Gauge_desc; /* PRIV */

#endif /* _MUI_CLASSES_GAUGE_H */
