#ifndef AROS_PURIFY_H
#define AROS_PURIFY_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Purify - a tool to check memory accesses
*/

#ifndef ENABLE_PURIFY
#   define ENABLE_PURIFY    0
#endif

#define PMS_FREE	0
#define PMS_EMPTY	1
#define PMS_INITIALIZED 2
#define PMS_READONLY	3

#define PMA_READ	0
#define PMA_WRITE	1
#define PMA_MODIFY	2   /* READ+WRITE */

void Purify_Init (void);
void Purify_AddMemory (APTR memPtr, ULONG size);
void Purify_SetState (APTR memPtr, ULONG size, ULONG state);
void Purify_CheckAccess (APTR memPtr, ULONG size, ULONG type);

#endif /* AROS_PURIFY_H */
