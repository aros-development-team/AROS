/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_DATASPACE_H
#define _MUI_CLASSES_DATASPACE_H

#define MUIC_Dataspace "Dataspace.mui"

/* Datatspace methods */
#define MUIM_Dataspace_Add        (METHOD_USER|0x00423366) /* MUI: V11 */
#define MUIM_Dataspace_Clear      (METHOD_USER|0x0042b6c9) /* MUI: V11 */
#define MUIM_Dataspace_Find       (METHOD_USER|0x0042832c) /* MUI: V11 */
#define MUIM_Dataspace_Merge      (METHOD_USER|0x00423e2b) /* MUI: V11 */
#define MUIM_Dataspace_ReadIFF    (METHOD_USER|0x00420dfb) /* MUI: V11 */
#define MUIM_Dataspace_Remove     (METHOD_USER|0x0042dce1) /* MUI: V11 */
#define MUIM_Dataspace_WriteIFF   (METHOD_USER|0x00425e8e) /* MUI: V11 */
struct MUIP_Dataspace_Add         {ULONG MethodID; APTR data; LONG len; ULONG id;};
struct MUIP_Dataspace_Clear       {ULONG MethodID;};
struct MUIP_Dataspace_Find        {ULONG MethodID; ULONG id;};
struct MUIP_Dataspace_Merge       {ULONG MethodID; Object *dataspace;};
struct MUIP_Dataspace_ReadIFF     {ULONG MethodID; struct IFFHandle *handle;};
struct MUIP_Dataspace_Remove      {ULONG MethodID; ULONG id;};
struct MUIP_Dataspace_WriteIFF    {ULONG MethodID; struct IFFHandle *handle; ULONG type; ULONG id;};

/* Dataspace attributes */
#define MUIA_Dataspace_Pool       (TAG_USER|0x00424cf9) /* MUI: V11 i.. APTR */

extern const struct __MUIBuiltinClass _MUI_Dataspace_desc; /* PRIV */

#endif
