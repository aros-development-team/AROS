/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>

void crashfunc()
{
    volatile LONG * ptr = NULL;

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
