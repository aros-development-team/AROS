#ifndef _POOL_H
#define _POOL_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Original version from libnix
    $Id$

    Desc: amiga.lib internal header file for pools
    Lang: english
*/
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>

/*     our PRIVATE! memory pool structure
   (_NOT_ compatible with original amiga.lib!) */

typedef struct Pool
{
  struct MinList PuddleList;
  struct MinList ThreshList;

  ULONG MemoryFlags;
  ULONG PuddleSize;
  ULONG ThreshSize;
} POOL;

#endif /* _POOL_H */
