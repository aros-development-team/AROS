/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_SCROLLBUTTON_H
#define _MUI_CLASSES_SCROLLBUTTON_H

#define MUIC_Scrollbutton "Scrollbutton.mui"

#define ScrollbuttonObject MUI_NewObject(MUIC_Scrollbutton

#define MUIA_Scrollbutton_NewPosition		(TAG_USER|0x1820a000) /* ..G ULONG (packed two WORDs)*/
#define MUIA_Scrollbutton_Horiz					(TAG_USER|0x1820a001) /*     WORD */
#define MUIA_Scrollbutton_Vert						(TAG_USER|0x1820a002) /*     WORD */
#define MUIA_Scrollbutton_HorizProp       (TAG_USER|0x1820a003) /* ISG Object * */
#define MUIA_Scrollbutton_VertProp        (TAG_USER|0x1820a004) /* ISG Object * */

extern const struct __MUIBuiltinClass _MUI_Scrollbutton_desc; /* PRIV */

#endif  /* FC__SCROLLBUTTONCLASS_H */
