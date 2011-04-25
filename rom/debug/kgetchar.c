#include <proto/debug.h>
#include <proto/exec.h>

LONG KGetChar(VOID)
{
    LONG c;
    
    do
        c = RawMayGetChar();
    while (c == -1);

    return c;
}
