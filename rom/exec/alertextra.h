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

struct MungwallHeader;

struct MungwallContext
{
    struct MungwallHeader *hdr;
    const char 		  *freeFunc;
    IPTR		   freeSize;
    APTR		   pre_start;
    APTR		   pre_end;
    APTR		   post_start;
    APTR		   post_end;
};

char *FormatMMContext(char *buffer, struct MMContext *ctx, struct ExecBase *SysBase);
char *FormatMWContext(char *buffer, struct MungwallContext *ctx, struct ExecBase *SysBase);
