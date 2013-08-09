#include <proto/dos.h>
#include <dos/stdio.h>

int main(void)
{
    FPuts(Output(), "The command line arguments should be printed on next line\n");

    UBYTE c;
    BPTR in = Input(), out = Output();
    for (c = FGetC(in);
         c != '\n' && c != ENDSTREAMCH;
         c = FGetC(in)
    )
        FPutC(out, c);
    FPutC(out, '\n');

    return 0;
}
