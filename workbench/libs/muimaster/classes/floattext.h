#ifndef _MUI_CLASSES_FLOATTEXT_H
#define _MUI_CLASSES_FLOATTEXT_H

/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Floattext           "Floattext.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Floattext           (MUIB_ZUNE | 0x00001500)

/*** Attributes *************************************************************/
#define MUIA_Floattext_Justify \
    (MUIB_MUI | 0x0042dc03)  /* MUI: V4  isg BOOL   */
#define MUIA_Floattext_SkipChars \
    (MUIB_MUI | 0x00425c7d)  /* MUI: V4  is. STRPTR */
#define MUIA_Floattext_TabSize \
    (MUIB_MUI | 0x00427d17)  /* MUI: V4  is. LONG   */
#define MUIA_Floattext_Text \
    (MUIB_MUI | 0x0042d16a)  /* MUI: V4  isg STRPTR */

extern const struct __MUIBuiltinClass _MUI_Floattext_desc;      /* PRIV */

#endif /* _MUI_CLASSES_FLOATTEXT_H */
