/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_BOOPSI_H
#define _MUI_CLASSES_BOOPSI_H

#define MUIC_Boopsi "Boopsi.mui"

/* Boopsi attributes */
#define MUIA_Boopsi_Class       (TAG_USER|0x00426999) /* V4  isg struct IClass * */
#define MUIA_Boopsi_ClassID     (TAG_USER|0x0042bfa3) /* V4  isg char *          */
#define MUIA_Boopsi_MaxHeight   (TAG_USER|0x0042757f) /* V4  isg ULONG           */
#define MUIA_Boopsi_MaxWidth    (TAG_USER|0x0042bcb1) /* V4  isg ULONG           */
#define MUIA_Boopsi_MinHeight   (TAG_USER|0x00422c93) /* V4  isg ULONG           */
#define MUIA_Boopsi_MinWidth    (TAG_USER|0x00428fb2) /* V4  isg ULONG           */
#define MUIA_Boopsi_Object      (TAG_USER|0x00420178) /* V4  ..g Object *        */
#define MUIA_Boopsi_Remember    (TAG_USER|0x0042f4bd) /* V4  i.. ULONG           */
#define MUIA_Boopsi_Smart       (TAG_USER|0x0042b8d7) /* V9  i.. BOOL            */
#define MUIA_Boopsi_TagDrawInfo (TAG_USER|0x0042bae7) /* V4  isg ULONG           */
#define MUIA_Boopsi_TagScreen   (TAG_USER|0x0042bc71) /* V4  isg ULONG           */
#define MUIA_Boopsi_TagWindow   (TAG_USER|0x0042e11d) /* V4  isg ULONG           */

/* The following tag is Zune Private */
#define MUIA_Boopsi_OnlyTrigger (TAG_USER|0x1042e11e) /* ZV1 PRIV (for notification) */

extern const struct __MUIBuiltinClass _MUI_Boopsi_desc; /* PRIV */

#endif
