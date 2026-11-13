#ifndef _MUI_CLASSES_DTPIC_H
#define _MUI_CLASSES_DTPIC_H

/*
    Copyright © 2002-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Dtpic          "Dtpic.mui"

/*** Attributes *************************************************************/
#define MUIA_Dtpic_Alpha \
    (MUIB_MUI | 0x0042b4db) /* MUI: V20 isg LONG   */
#define MUIA_Dtpic_DarkenSelState \
    (MUIB_MUI | 0x00423247) /* MUI: V20 i.g BOOL   */
#define MUIA_Dtpic_Fade \
    (MUIB_MUI | 0x00420429) /* MUI: V20 isg LONG   */
#define MUIA_Dtpic_LightenOnMouse \
    (MUIB_MUI | 0x0042966a) /* MUI: V20 i.g BOOL   */
#define MUIA_Dtpic_Name \
    (MUIB_MUI | 0x00423d72) /* MUI: V18 isg STRPTR */

extern const struct __MUIBuiltinClass _MUI_Dtpic_desc;  /* PRIV */

#endif /* _MUI_CLASSES_DTPIC_H */
