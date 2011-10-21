#include <exec/execbase.h>
#include <exec/memory.h>

struct MMContext
{
    struct MemHeader *mh;
    struct MemChunk  *mc;
    struct MemChunk  *mcPrev;
    const char	     *func;
    APTR	      addr;
    IPTR	      size;
    UBYTE	      op;
};

#define MM_ALLOC   0
#define MM_FREE    1
#define MM_OVERLAP 2

char *FormatMMContext(char *buffer, struct MMContext *ctx, struct ExecBase *SysBase);
