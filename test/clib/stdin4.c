#include <proto/dos.h>
#include <dos/stdio.h>
#include <errno.h>
#include <stdio.h>

int main(void)
{
    FPuts(Output(), "The command line arguments should be printed on next line\n");

    LONG c;
    BPTR in = Input(), out = Output();
    for (c = FGetC(in);
         c != '\n' && c != ENDSTREAMCH;
         c = FGetC(in)
    )
        FPutC(out, c);
    FPutC(out, '\n');

    puts("Type a line and it should be repeated");

    char c2;
    for (c2 = getc(stdin);
         c2 != '\n' && c2 != EOF && errno == 0;
         c2 = getc(stdin)
    )
        putc(c2, stdout);
    putc('\n', stdout);

    return 0;
}
