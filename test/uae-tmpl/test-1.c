
#include <stdio.h>
#include <assert.h>

#include "aros_types.h"

int main (int argc, char ** argv)
{
    short	   s;
    unsigned short us;
    WORD	   w;
    UWORD	   uw;
    ULONG	   ul;

    s  = 0;
    us = 0;
    w  = 0;
    uw = 0;

    assert (s == w);
    assert (us == uw);

    s  = 0x1234;
    us = 0x1234;
    w  = 0x1234;
    uw = 0x1234;
    ul = 0x1234;

    assert (s == w);
    printf ("us=%x uw=%x\n", us, short (uw));
    printf ("ul=%lx\n", (unsigned long)ul);
    assert (us == uw);

    s  = w;
    us = uw;
    w  = s;
    uw = us;

    assert (s == w);
    assert (us == uw);
}
