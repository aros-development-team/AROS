/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library private memory management
*/
#ifndef _SECURITY_MEMORY_H
#define _SECURITY_MEMORY_H

#include <exec/types.h>

struct SecurityBase;

extern BOOL InitMemory(struct SecurityBase *secBase);
extern void CleanUpMemory(struct SecurityBase *secBase);
extern APTR MAlloc(ULONG size);
extern void Free(APTR block, ULONG size);
extern APTR MAllocV(ULONG size);
extern void FreeV(APTR block);

#endif /* _SECURITY_MEMORY_H */
