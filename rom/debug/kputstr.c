#include <proto/debug.h>
#include <proto/exec.h>

VOID KPutStr(CONST_STRPTR string)
{
    while (*string)
    	RawPutChar(*string++);
}
