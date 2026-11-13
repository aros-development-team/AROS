/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _DRM_COMPAT_MEM_
#define _DRM_COMPAT_MEM_

#include <linux/slab.h>
#include <linux/mm.h>

/* the pool every allocation in the driver comes from */
APTR HIDDAmdgpuAlloc(ULONG size);
VOID HIDDAmdgpuFree(APTR memory);
IPTR HIDDAmdgpuAllocSize(CONST_APTR memory);

#endif /* _DRM_COMPAT_MEM_ */
