#include <proto/dos.h>
#include <dos/stdio.h>

int main(void)
{
    FPuts(Output(), "Type a line and it should be repeated\n");

    UBYTE c;
    BPTR in = Input(), out = Output();
    Flush(in);
    for (c = FGetC(in);
         c != '\n' && c != ENDSTREAMCH;
         c = FGetC(in)
    )
        FPutC(out, c);
    FPutC(out, '\n');

    return 0;
}
