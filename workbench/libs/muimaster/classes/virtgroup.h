#ifndef _MUI_CLASSES_VIRTGROUP_H
#define _MUI_CLASSES_VIRTGROUP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Virtgroup          "Virtgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Virtgroup          (MUIB_ZUNE | 0x00003700)  

/*** Attributes *************************************************************/
#define MUIA_Virtgroup_Height   (MUIB_MUI|0x00423038) /* V6  ..g LONG */
#define MUIA_Virtgroup_Input    (MUIB_MUI|0x00427f7e) /* V11 i.. BOOL */
#define MUIA_Virtgroup_Left     (MUIB_MUI|0x00429371) /* V6  isg LONG */
#define MUIA_Virtgroup_Top      (MUIB_MUI|0x00425200) /* V6  isg LONG */
#define MUIA_Virtgroup_Width    (MUIB_MUI|0x00427c49) /* V6  ..g LONG */

#define MUIA_Virtgroup_MinWidth    (MUIB_Virtgroup | 0x00000001) /* Zune: V1 ..g BOOL PRIV */
#define MUIA_Virtgroup_MinHeight   (MUIB_Virtgroup | 0x00000002) /* Zune: V1 ..g BOOL PRIV */


extern const struct __MUIBuiltinClass _MUI_Virtgroup_desc; /* PRIV */

#endif /* _MUI_CLASSES_VIRTGROUP_H */
