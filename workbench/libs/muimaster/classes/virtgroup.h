/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_VIRTGROUP_H
#define _MUI_CLASSES_VIRTGROUP_H

#define MUIC_Virtgroup "Virtgroup.mui"

/* Virtgroup attributes */
#define MUIA_Virtgroup_Height   (TAG_USER|0x00423038) /* V6  ..g LONG */
#define MUIA_Virtgroup_Input    (TAG_USER|0x00427f7e) /* V11 i.. BOOL */
#define MUIA_Virtgroup_Left     (TAG_USER|0x00429371) /* V6  isg LONG */
#define MUIA_Virtgroup_Top      (TAG_USER|0x00425200) /* V6  isg LONG */
#define MUIA_Virtgroup_Width    (TAG_USER|0x00427c49) /* V6  ..g LONG */

extern const struct __MUIBuiltinClass _MUI_Virtgroup_desc; /* PRIV */

#endif
