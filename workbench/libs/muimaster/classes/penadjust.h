#ifndef _MUI_CLASSES_PENADJUST_H
#define _MUI_CLASSES_PENADJUST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Penadjust "Penadjust.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Penadjust         (MUIB_ZUNE | 0x00001f00)  

/*** Attributes *************************************************************/
#define MUIA_Penadjust_PSIMode (MUIB_MUI|0x00421cbb) /* MUI: V11  i.. BOOL       */

#define MUIA_Penadjust_Spec    (MUIB_MUI|0x0062339b) /* isg  struct MUI_Penspec *  PRIV */


extern const struct __MUIBuiltinClass _MUI_Penadjust_desc; /* PRIV */

#endif /* _MUI_CLASSES_PENADJUST_H */
