 /***************************************************************************\
|*                                                                           *|
|*       Copyright 1993-1999 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 1993-1999 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/

#ifndef __NV_LOCAL_H__
#define __NV_LOCAL_H__

/*
 * This file includes any environment or machine specific values to access the
 * HW.  Put all affected includes, typdefs, etc. here so the riva_hw.* files
 * can stay generic in nature.
 */ 

#include <exec/types.h>
#include "nv.h"

/*
 * Typedefs to force certain sized values.
 */
typedef UBYTE	U008;
typedef UWORD	U016;
typedef ULONG   U032;

typedef UBYTE	CARD8;
typedef BOOL	Bool;

/*
 * HW access macros.  These assume memory-mapped I/O, and not normal I/O space.
 */
#define NV_WR08(p,i,d)  (*(volatile UBYTE*)((UBYTE*)(p)+(i)) = (d))
#define NV_RD08(p,i)    (*(volatile UBYTE*)((UBYTE*)(p)+(i)))
#define NV_WR16(p,i,d)  (*(volatile UWORD*)((UBYTE*)(p)+(i)) = (d))
#define NV_RD16(p,i)    (*(volatile UWORD*)((UBYTE*)(p)+(i)))
#define NV_WR32(p,i,d)  (*(volatile ULONG*)((UBYTE*)(p)+(i)) = (d))
#define NV_RD32(p,i)    (*(volatile ULONG*)((UBYTE*)(p)+(i)))

/* VGA I/O is now always done through MMIO */
#define VGA_WR08(p,i,d) NV_WR08(p,i,d)
#define VGA_RD08(p,i)   NV_RD08(p,i)

#endif /* __NV_LOCAL_H__ */
