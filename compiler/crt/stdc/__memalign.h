#ifndef ___MEMALIGN_H
#define ___MEMALIGN_H

#include <aros/cpu.h>

/* malloc() keeps the size of a block in front of it. The pools hand out
   chunks aligned to at least 8 bytes (MEMPOOL_WORSTALIGN in exec), so the
   prefix is rounded to that alignment rather than to AROS_WORSTALIGN: the
   block then keeps the chunk's alignment, which is what code written for
   glibc-style allocators assumes (8 bytes on 32-bit targets). Only m68k,
   where AROS_WORSTALIGN is 4, is affected; everywhere else the prefix is
   the same size as before. */
#define MALLOC_ALIGN   (AROS_WORSTALIGN > 8 ? AROS_WORSTALIGN : 8)
#define MALLOC_HDRSIZE ((sizeof(size_t) + MALLOC_ALIGN - 1) & -MALLOC_ALIGN)

/* Marks a block from aligned_alloc(), whose size slot cannot hold a size:
   the pointer to the malloc() block it lies in sits in front of the mark. */
#define MEMALIGN_MAGIC ((size_t) 0x57E2CB09)

#endif
