/* 
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_IMAGEADJUST_H
#define _MUI_CLASSES_IMAGEADJUST_H

#define MUIC_Imageadjust "Imageadjust.mui"

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

#define MUIA_Imageadjust_Type (TAG_USER|0x00422f2b) /* MUI: V11 i.. LONG */
#define MUIA_Imageadjust_Spec (TAG_USER|0x004279e1) /* MUI: ??? .g. char * */

enum
{
    MUIV_Imageadjust_Type_All = 0,
    MUIV_Imageadjust_Type_Image,
    MUIV_Imageadjust_Type_Background,
    MUIV_Imageadjust_Type_Pen,
};

extern const struct __MUIBuiltinClass _MUI_Imageadjust_desc; /* PRIV */

#define MUIM_Imageadjust_ReadExternal (METHOD_USER|0x10101010) /* PRIV */

#endif
