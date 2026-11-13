/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>

void crashfunc()
{
#if __WORDSIZE==64
    volatile LONG *ptr = (LONG *)0xFFFF000000000000ULL;
#else
volatile LONG *ptr = (LONG *)0xFFFF0000UL;
#endif

    PutStr("Before crash\n");
    *ptr = 20;
    PutStr("After crash\n");
}

int main(int argc, char **argv)
{
    PutStr("main() enter\n");
    crashfunc();
    PutStr("main() exit\n");
    return 0;
}
