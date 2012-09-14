#ifndef _MUI_CLASSES_FRAMEADJUST_H
#define _MUI_CLASSES_FRAMEADJUST_H

/* 
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Frameadjust "Frameadjust.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Frameadjust      (MUIB_ZUNE | 0x00000d00)

/*** Attributes *************************************************************/
#define MUIA_Frameadjust_Spec \
    (MUIB_Frameadjust | 0x00000000)   /* Zune 20030330 ig. CONST_STRPTR */


extern const struct __MUIBuiltinClass _MUI_Frameadjust_desc;    /* PRIV */

#endif /* _MUI_CLASSES_FRAMEADJUST_H */
