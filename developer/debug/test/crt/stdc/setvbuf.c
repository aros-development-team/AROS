/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: Test calling setvbuf.
*/

#include <stdio.h>

int main(void)
{
    char buf[1024];

    if (setvbuf(stdout, buf, _IOFBF, 1024) == 0)
    {
        printf("\033[32mBuffered output with escape codes\033[0m\n");

    }
    else
    {
        printf("\033[32msetvbuf(stdout, buf, _IOFBF, 1024); failed\033[0m\n");
    }

    fflush( stdout );

    if (setvbuf(stdout, NULL, _IONBF, 0) == 0)
    {
        printf("\033[32mUnbuffered output with escape codes\033[0m\n");
    }
    else
    {
        printf("\033[32msetvbuf(stdout, NULL, _IONBF, 0); failed\033[0m\n");
    }

    return 0;
}
