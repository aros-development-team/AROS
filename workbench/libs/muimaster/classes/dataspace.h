/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_DATASPACE_H
#define _MUI_CLASSES_DATASPACE_H

/****************************************************************************/
/** Dataspace                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Dataspace[];
#else
#define MUIC_Dataspace "Dataspace.mui"
#endif

/* Methods */

#define MUIM_Dataspace_Add                  0x80423366 /* V11 */
#define MUIM_Dataspace_Clear                0x8042b6c9 /* V11 */
#define MUIM_Dataspace_Find                 0x8042832c /* V11 */
#define MUIM_Dataspace_Merge                0x80423e2b /* V11 */
#define MUIM_Dataspace_ReadIFF              0x80420dfb /* V11 */
#define MUIM_Dataspace_Remove               0x8042dce1 /* V11 */
#define MUIM_Dataspace_WriteIFF             0x80425e8e /* V11 */
struct  MUIP_Dataspace_Add                  { ULONG MethodID; APTR data; LONG len; ULONG id; };
struct  MUIP_Dataspace_Clear                { ULONG MethodID; };
struct  MUIP_Dataspace_Find                 { ULONG MethodID; ULONG id; };
struct  MUIP_Dataspace_Merge                { ULONG MethodID; Object *dataspace; };
struct  MUIP_Dataspace_ReadIFF              { ULONG MethodID; struct IFFHandle *handle; };
struct  MUIP_Dataspace_Remove               { ULONG MethodID; ULONG id; };
struct  MUIP_Dataspace_WriteIFF             { ULONG MethodID; struct IFFHandle *handle; ULONG type; ULONG id; };

/* Attributes */

#define MUIA_Dataspace_Pool                 0x80424cf9 /* V11 i.. APTR              */

extern const struct __MUIBuiltinClass _MUI_Dataspace_desc;

#endif
