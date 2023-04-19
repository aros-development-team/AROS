/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: Test calling setbuf.
*/

#include <stdio.h>

int main(void)
{
    setbuf(stdout, NULL);

    return 0;
}
