
#include <stdio.h>
#include "types.h"

int main (int argc, char ** argv)
{
    int a;
    short b;
    WORD c;

    c = 15;
    a = c;
    b = c;

    printf ("%d %d %d\n", a, b, (int)c);

    void * p1;
    APTR p2;

    p1 = "hello";
    p2 = p1;

    printf ("%p %p\n", p1, (void*)p2);

    return 0;
}
