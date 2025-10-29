/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>

void crashfunc()
{
    void (*bad)() = (void(*)())"abcd";
    PutStr("Before crash\n");
    bad();
    PutStr("After crash\n");
}

int main(int argc, char **argv)
{
    PutStr("main() enter\n");
    crashfunc();
    PutStr("main() exit\n");
    return 0;
}
