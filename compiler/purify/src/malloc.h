#ifndef MALLOC_H /* must be MALLOC_H to avoid conflicts with <malloc.h> */
#define MALLOC_H

#ifndef _POSINFO_H
#   include "posinfo.h"
#endif

typedef struct _PMemoryNode PMemoryNode;

struct _PMemoryNode
{
    RememberData alloc;
    RememberData free;
    char       * memptr;
    char	 mem[16];
};

void Purify_MemoryExit (void);

#endif /* MALLOC_H */
