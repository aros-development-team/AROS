/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_SCROLLBUTTON_H
#define _MUI_CLASSES_SCROLLBUTTON_H

/*** Name *******************************************************************/
#define MUIC_Scrollbutton             "Scrollbutton.mui"

/*** Identifier base ********************************************************/
#define MUIB_Scrollbutton             (MUIB_ZUNE | 0x00004100)  

/*** Attributes *************************************************************/
#define MUIA_Scrollbutton_NewPosition (MUIB_Scrollbutton | 0x00000000) /* --G  ULONG (2 x WORD) */
#define MUIA_Scrollbutton_Horiz       (MUIB_Scrollbutton | 0x00000001) /* -SG  WORD             */
#define MUIA_Scrollbutton_Vert        (MUIB_Scrollbutton | 0x00000002) /* -SG  WORD             */
#define MUIA_Scrollbutton_HorizProp   (MUIB_Scrollbutton | 0x00000003) /* --G  Object *         */
#define MUIA_Scrollbutton_VertProp    (MUIB_Scrollbutton | 0x00000004) /* --G  Object *         */

/*** Macros *****************************************************************/
#define ScrollbuttonObject MUIOBJMACRO_START(MUIC_Scrollbutton)

extern const struct __MUIBuiltinClass _MUI_Scrollbutton_desc; /* PRIV */

#endif  /* _MUI_CLASSES_SCROLLBUTTON_H */
