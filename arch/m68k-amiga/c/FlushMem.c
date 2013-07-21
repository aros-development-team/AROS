#include <proto/dos.h>
#include <proto/exec.h>

#include <aros/shcommands.h>

#define BUG(x) do { const char *cp = x; while (*cp) { RawPutChar(*cp); cp++; } } while (0)

void flushall(struct ExecBase *SysBase)
{
    APTR ptr;
    ptr = AllocMem(0x7fffffff, MEMF_ANY);
    if (ptr) {
        flushall(SysBase);
        FreeMem(ptr, 0x7fffffff);
    }
}

AROS_SH0H(FlushMem, 1.0, "Flush all expungable memory")
{
    AROS_SHCOMMAND_INIT

    BUG("---- FlushMem: Begin\n");
    flushall(SysBase);
    BUG("---- FlushMem: End\n");
    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
