#ifndef _MEMORY_H
#define _MEMORY_H

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

#endif /* _MEMORY_H */
