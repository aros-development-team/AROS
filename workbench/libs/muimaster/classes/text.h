/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_TEXT_H
#define _MUI_CLASSES_TEXT_H

#ifndef _MUI_TEXTENGINE_H
#include "textengine.h"
#endif

#define MUIC_Text "Text.mui"

/* Text attributes */
#define MUIA_Text_Contents  (TAG_USER|0x0042f8dc) /* MUI: V4  isg STRPTR */
#define MUIA_Text_HiChar    (TAG_USER|0x004218ff) /* MUI: V4  i.. char   */
#define MUIA_Text_HiCharIdx (TAG_USER|0x004214f5) /*          i.. char   */
#define MUIA_Text_PreParse  (TAG_USER|0x0042566d) /* MUI: V4  isg STRPTR */
#define MUIA_Text_SetMax    (TAG_USER|0x00424d0a) /* MUI: V4  i.. BOOL   */
#define MUIA_Text_SetMin    (TAG_USER|0x00424e10) /* MUI: V4  i.. BOOL   */
#define MUIA_Text_SetVMax   (TAG_USER|0x00420d8b) /* MUI: V11 i.. BOOL   */

/* The following stuff in new for Zune */
#define MUIA_Text_Editable    (TAG_USER|0x00420d8c)  /* Zune: V1 i.. BOOL */
#define MUIA_Text_Multiline   (TAG_USER|0x00420d8d)  /* Zune: V1 i.. BOOL */

extern const struct __MUIBuiltinClass _MUI_Text_desc; /* PRIV */

#endif
