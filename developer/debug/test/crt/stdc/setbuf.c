/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: Test calling setbuf.
*/

#include <stdio.h>

int main(void)
{
   char buf[BUFSIZ];

    setbuf(stdout, buf);
    printf("\033[32mBuffered output with escape codes\033[0m\n");
    fflush( stdout );

    setbuf(stdout, NULL);
    printf("\033[32mUnbuffered output with escape codes\033[0m\n");

    return 0;
}
