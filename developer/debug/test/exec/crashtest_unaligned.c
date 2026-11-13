/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>

/* on strict-alignment architectures this should crash */
void crashfunc()
{
    char buf[5];
    LONG *unaligned = (LONG *)(buf + 1);
    PutStr("Before crash\n");
    *unaligned = 0x12345678;
    PutStr("After crash\n");
}

int main(int argc, char **argv)
{
    PutStr("main() enter\n");
    crashfunc();
    PutStr("main() exit\n");
    return 0;
}
