/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>

void crashfunc()
{
    volatile int x = 1;
    volatile int y = 0;
    PutStr("Before crash\n");
    volatile int z = x / y;
    PutStr("After crash\n");
    (void)z;
}

int main(int argc, char **argv)
{
    PutStr("main() enter\n");
    crashfunc();
    PutStr("main() exit\n");
    return 0;
}
