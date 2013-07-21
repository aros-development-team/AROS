#include <proto/dos.h>
#include <proto/exec.h>

#include <aros/shcommands.h>

AROS_SH0H(GdbStop, 1.0, "Issue a debugger breakpoint")
{
    AROS_SHCOMMAND_INIT

    asm volatile ("trap #1\n");
    PutStr("Back from trap #1 breakpoint\n");
    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
