/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_ICONLISTVIEW_H
#define _MUI_CLASSES_ICONLISTVIEW_H

/*** Name *******************************************************************/
#define MUIC_IconListview               "IconListview.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconListview               (MUIB_ZUNE | 0x00004500)

/*** Attributes *************************************************************/
#define MUIA_IconListview_IconList      (MUIB_IconListview | 0x00000000) /* Zune: V1  i.g Object * */
#define MUIA_IconListview_UseWinBorder  (MUIB_IconListview | 0x00000001) /* Zune: V1  i.. BOOL     */


extern const struct __MUIBuiltinClass _MUI_IconListview_desc; /* PRIV */

#endif /*_MUI_CLASSES_ICONLISTVIEW_H */
