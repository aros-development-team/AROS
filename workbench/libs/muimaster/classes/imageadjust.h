#ifndef _MUI_CLASSES_IMAGEADJUST_H
#define _MUI_CLASSES_IMAGEADJUST_H

/* 
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Imageadjust      "Imageadjust.mui"

/*** Identifier base (for AROS extensions) **********************************/
#define MUIB_Imageadjust      (MUIB_ZUNE | 0x00001100)  

/*** Attributes *************************************************************/
#define MUIA_Imageadjust_Type (MUIB_MUI|0x00422f2b) /* MUI: V11 i.. LONG */
#define MUIA_Imageadjust_Spec (MUIB_MUI|0x004279e1) /* MUI: ??? .g. char * */
#define MUIA_Imageadjust_Originator (MUIB_Imageadjust|0x00000000) /* Zune: i.. Object * */

enum
{
    MUIV_Imageadjust_Type_All = 0,
    MUIV_Imageadjust_Type_Image,
    MUIV_Imageadjust_Type_Background,
    MUIV_Imageadjust_Type_Pen,
};

/*** Methods ****************************************************************/
#define MUIM_Imageadjust_ReadExternal (MUIB_Imageadjust | 0x00000000) /* PRIV */

extern const struct __MUIBuiltinClass _MUI_Imageadjust_desc; /* PRIV */

#endif /* _MUI_CLASSES_IMAGEADJUST_H */
