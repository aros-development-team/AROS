/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: Test calling setvbuf.
*/

#include <stdio.h>

int main(void)
{
    int result;

    result = setvbuf(stdout, NULL, _IONBF, 0);
    printf("setvbuf returned %d\n", result);

    return 0;
}
