#include <stdio.h>

int c;

void foo (int * x)
{
    *x ++ = 1;
}

int main (int argc, char ** argv)
{
    int * x;
    int a, b;

    x = &b;

    a = *x; /* Uninitialized memory read */

    return 0;
}
