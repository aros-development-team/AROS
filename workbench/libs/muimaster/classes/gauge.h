/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_GAUGE
#define _MUI_CLASSES_GAUGE

#define MUIC_Gauge "Gauge.mui"

/* Gauge attributes */
#define MUIA_Gauge_Current  (TAG_USER|0x0042f0dd) /* MUI: V4  isg LONG    */
#define MUIA_Gauge_Divide   (TAG_USER|0x0042d8df) /* MUI: V4  isg LONG    */
#define MUIA_Gauge_Horiz    (TAG_USER|0x004232dd) /* MUI: V4  i.. BOOL    */
#define MUIA_Gauge_InfoText (TAG_USER|0x0042bf15) /* MUI: V7  isg STRPTR  */
#define MUIA_Gauge_Max      (TAG_USER|0x0042bcdb) /* MUI: V4  isg LONG    */

#define MUIA_Gauge_DupInfoText (TAG_USER|0x1000e302) /* ZUNE: V1  i.. BOOL - defaults to FALSE */

extern const struct __MUIBuiltinClass _MUI_Gauge_desc; /* PRIV */

#endif
