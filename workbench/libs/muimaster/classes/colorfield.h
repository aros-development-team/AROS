/*
    Copyright © 2002-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_COLORFIELD_H
#define _MUI_CLASSES_COLORFIELD_H

/*** Name *******************************************************************/
#define MUIC_Colorfield "Colorfield.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Colorfield         (MUIB_ZUNE | 0x00000800)

/*** Attributes *************************************************************/
#define MUIA_Colorfield_Pen         (MUIB_MUI | 0x0042713a) /* isg  ULONG   */
#define MUIA_Colorfield_Red         (MUIB_MUI | 0x004279f6) /* isg  ULONG   */
#define MUIA_Colorfield_Green       (MUIB_MUI | 0x00424466) /* isg  ULONG   */
#define MUIA_Colorfield_Blue        (MUIB_MUI | 0x0042d3b0) /* isg  ULONG   */
#define MUIA_Colorfield_RGB         (MUIB_MUI | 0x0042677a) /* isg  ULONG * */


extern const struct __MUIBuiltinClass _MUI_Colorfield_desc; /* PRIV */

#endif /* _MUI_CLASSES_COLORFIELD_H */
