#ifndef EXEC_MEMHEADEREXT_H
#define EXEC_MEMHEADEREXT_H
/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
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
       
    APTR  mhe_UserData;

    /* Pool initialization and destruction */
    APTR  (* mhe_InitPool)(struct MemHeaderExt *, IPTR initialSize);
    APTR  (* mhe_DestroyPool)(struct MemHeaderExt *);

    /* Memory allocation functions */
    APTR  (* mhe_Alloc)   (struct MemHeaderExt *, IPTR  size,  ULONG *flags);
    VOID  (* mhe_Free)    (struct MemHeaderExt *, APTR  mem,   IPTR   size);
    APTR  (* mhe_AllocVec)(struct MemHeaderExt *, IPTR  size,  ULONG *flags);
    VOID  (* mhe_FreeVec) (struct MemHeaderExt *, APTR  mem);
    APTR  (* mhe_AllocAbs)(struct MemHeaderExt *, IPTR  size,  APTR   addr);
    APTR  (* mhe_ReAlloc) (struct MemHeaderExt *, APTR  old,   IPTR   size);

    /* Query functions */
    ULONG (* mhe_Avail)   (struct MemHeaderExt *, ULONG flags);
    BOOL  (* mhe_InBounds)(struct MemHeaderExt *, APTR  begin, APTR   end);
};

/* Indicates that the memory region is to be
   treated as an opaque object managed only through the
   functions whose pointers are in the extended mem header.  */
#define MEMF_MANAGED  (1L << 15)

#endif
