#ifndef _MUI_CLASSES_DATASPACE_H
#define _MUI_CLASSES_DATASPACE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Dataspace            "Dataspace.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Dataspace            (MUIB_ZUNE | 0x00000b00)


/*** Methods ****************************************************************/
#define MUIM_Dataspace_Add        (MUIB_MUI|0x00423366) /* MUI: V11 */
#define MUIM_Dataspace_Clear      (MUIB_MUI|0x0042b6c9) /* MUI: V11 */
#define MUIM_Dataspace_Find       (MUIB_MUI|0x0042832c) /* MUI: V11 */
#define MUIM_Dataspace_Merge      (MUIB_MUI|0x00423e2b) /* MUI: V11 */
#define MUIM_Dataspace_ReadIFF    (MUIB_MUI|0x00420dfb) /* MUI: V11 */
#define MUIM_Dataspace_Remove     (MUIB_MUI|0x0042dce1) /* MUI: V11 */
#define MUIM_Dataspace_WriteIFF   (MUIB_MUI|0x00425e8e) /* MUI: V11 */
struct MUIP_Dataspace_Add         {ULONG MethodID; APTR data; LONG len; ULONG id;};
struct MUIP_Dataspace_Clear       {ULONG MethodID;};
struct MUIP_Dataspace_Find        {ULONG MethodID; ULONG id;};
struct MUIP_Dataspace_Merge       {ULONG MethodID; Object *dataspace;};
struct MUIP_Dataspace_ReadIFF     {ULONG MethodID; struct IFFHandle *handle;};
struct MUIP_Dataspace_Remove      {ULONG MethodID; ULONG id;};
struct MUIP_Dataspace_WriteIFF    {ULONG MethodID; struct IFFHandle *handle; ULONG type; ULONG id;};

/*** Attributes *************************************************************/
#define MUIA_Dataspace_Pool       (MUIB_MUI|0x00424cf9) /* MUI: V11 i.. APTR */


extern const struct __MUIBuiltinClass _MUI_Dataspace_desc; /* PRIV */

#endif /* _MUI_CLASSES_DATASPACE_H */
