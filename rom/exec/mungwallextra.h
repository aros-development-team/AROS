#include <exec/execbase.h>

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

char *FormatMWContext(char *buffer, struct MungwallContext *ctx, struct ExecBase *SysBase);
