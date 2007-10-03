#ifndef _MUI_CLASSES_SEMAPHORE_H
#define _MUI_CLASSES_SEMAPHORE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Semaphore "Semaphore.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Semaphore (MUIB_ZUNE | 0x00003000)

/*** Methods ****************************************************************/
#define MUIM_Semaphore_Attempt       (MUIB_MUI|0x00426ce2) /* MUI: V11 */
#define MUIM_Semaphore_AttemptShared (MUIB_MUI|0x00422551) /* MUI: V11 */
#define MUIM_Semaphore_Obtain        (MUIB_MUI|0x004276f0) /* MUI: V11 */
#define MUIM_Semaphore_ObtainShared  (MUIB_MUI|0x0042ea02) /* MUI: V11 */
#define MUIM_Semaphore_Release       (MUIB_MUI|0x00421f2d) /* MUI: V11 */
struct MUIP_Semaphore_Attempt        {STACKED ULONG MethodID;};
struct MUIP_Semaphore_AttemptShared  {STACKED ULONG MethodID;};
struct MUIP_Semaphore_Obtain         {STACKED ULONG MethodID;};
struct MUIP_Semaphore_ObtainShared   {STACKED ULONG MethodID;};
struct MUIP_Semaphore_Release        {STACKED ULONG MethodID;};


extern const struct __MUIBuiltinClass _MUI_Semaphore_desc; /* PRIV */

#endif /* _MUI_CLASSES_SEMAPHORE_H */
