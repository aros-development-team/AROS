#ifndef _MUI_CLASSES_SCALE_H
#define _MUI_CLASSES_SCALE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Scale          "Scale.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scale          (MUIB_ZUNE | 0x00002d00)  

/*** Attributes *************************************************************/
#define MUIA_Scale_Horiz    (MUIB_MUI|0x0042919a) /* MUI: V4  isg BOOL    */


extern const struct __MUIBuiltinClass _MUI_Scale_desc; /* PRIV */

#endif /* _MUI_CLASSES_SCALE_H */
