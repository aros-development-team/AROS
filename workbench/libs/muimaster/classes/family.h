/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _CLASSES_FAMILY_H
#define _CLASSES_FAMILY_H

#define MUIC_Family "Family.mui"

/* Family methods */
#define MUIM_Family_AddHead    (MUIB_MUI|0x0042e200) /* MUI: V8  */
#define MUIM_Family_AddTail    (MUIB_MUI|0x0042d752) /* MUI: V8  */
#define MUIM_Family_Insert     (MUIB_MUI|0x00424d34) /* MUI: V8  */
#define MUIM_Family_Remove     (MUIB_MUI|0x0042f8a9) /* MUI: V8  */
#define MUIM_Family_Sort       (MUIB_MUI|0x00421c49) /* MUI: V8  */
#define MUIM_Family_Transfer   (MUIB_MUI|0x0042c14a) /* MUI: V8  */
struct MUIP_Family_AddHead     {ULONG MethodID; Object *obj;};
struct MUIP_Family_AddTail     {ULONG MethodID; Object *obj;};
struct MUIP_Family_Insert      {ULONG MethodID; Object *obj; Object *pred;};
struct MUIP_Family_Remove      {ULONG MethodID; Object *obj;};
struct MUIP_Family_Sort        {ULONG MethodID; Object *obj[1];};
struct MUIP_Family_Transfer    {ULONG MethodID; Object *family;};

/* Family attributes */
#define MUIA_Family_Child      (MUIB_MUI|0x0042c696) /* MUI: V8  i.. Object *          */
#define MUIA_Family_List       (MUIB_MUI|0x00424b9e) /* MUI: V8  ..g struct MinList *  */

extern const struct __MUIBuiltinClass _MUI_Family_desc; /* PRIV */

#endif
