/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_ICONLISTVIEW_H
#define _MUI_CLASSES_ICONLISTVIEW_H

#define MUIC_IconListview "IconListview.mui"

/* Scrollgroup attributes */
#define MUIA_IconListview_IconList     (TAG_USER|0x10421261) /* Zune: V1  i.g Object * */
#define MUIA_IconListview_HorizBar     (TAG_USER|0x0042b63d) /* Zune: V1  ..g Object * */
#define MUIA_IconListview_UseWinBorder (TAG_USER|0x004284c1) /* Zune: V1  i.. BOOL     */
#define MUIA_IconListview_VertBar      (TAG_USER|0x0042cdc0) /* Zune: V1 ..g Object *  */

extern const struct __MUIBuiltinClass _MUI_IconListview_desc; /* PRIV */

#endif
