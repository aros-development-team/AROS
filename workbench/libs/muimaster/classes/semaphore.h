/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_SEMAPHORE_H
#define _MUI_CLASSES_SEMAPHORE_H

/****************************************************************************/
/** Semaphore                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Semaphore[];
#else
#define MUIC_Semaphore "Semaphore.mui"
#endif

/* Methods */

#define MUIM_Semaphore_Attempt              0x80426ce2 /* V11 */
#define MUIM_Semaphore_AttemptShared        0x80422551 /* V11 */
#define MUIM_Semaphore_Obtain               0x804276f0 /* V11 */
#define MUIM_Semaphore_ObtainShared         0x8042ea02 /* V11 */
#define MUIM_Semaphore_Release              0x80421f2d /* V11 */

struct  MUIP_Semaphore_Attempt              { ULONG MethodID; };
struct  MUIP_Semaphore_AttemptShared        { ULONG MethodID; };
struct  MUIP_Semaphore_Obtain               { ULONG MethodID; };
struct  MUIP_Semaphore_ObtainShared         { ULONG MethodID; };
struct  MUIP_Semaphore_Release              { ULONG MethodID; };

/* Attributes */

extern const struct __MUIBuiltinClass _MUI_Semaphore_desc;

#endif
