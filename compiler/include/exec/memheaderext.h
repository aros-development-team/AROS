#ifndef EXEC_MEMHEADEREXT_H
#define EXEC_MEMHEADEREXT_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Extended memory handling. New in AROS.
    Lang: english
*/

#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif

struct MemHeaderExt
{
    struct MemHeader mhe_MemHeader;
            
    /* Let an external 'driver' manage this memory
       region opaquely.  */
       
    APTR  * mhe_UserData;
    
    APTR  (* mhe_Alloc)   (struct MemHeaderExt *, ULONG size, ULONG flags);
    VOID  (* mhe_Free)    (struct MemHeaderExt *, APTR  mem,  ULONG size);
    APTR  (* mhe_AllocAbs)(struct MemHeaderExt *, ULONG size, APTR  addr);
    APTR  (* mhe_ReAlloc) (struct MemHeaderExt *, APTR  old,  ULONG size);
    ULONG (* mhe_Avail)   (struct MemHeaderExt *, ULONG flags);
};

/* Indicates that the memory region is to be
   treated as an opaque object managed only through the
   functions whose pointers are in the extended mem header.  */
#define MEMF_MANAGED  (1L << 15)

#endif
