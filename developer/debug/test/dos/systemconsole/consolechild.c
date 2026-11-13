/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/debug.h>
#include <proto/dos.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    APTR expected, actual;

    if (argc != 2)
        return 20;
    expected = (APTR)(IPTR)strtoull(argv[1], NULL, 16);
    actual = GetConsoleTask();
    if (actual != expected)
    {
        bug("SYSTEM CONSOLE CHILD FAIL: expected %p, got %p\n", expected, actual);
        return 20;
    }
    return 0;
}
