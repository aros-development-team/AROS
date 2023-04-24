/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    Desc: Test calling setbuf.
*/

#include <stdio.h>

int main(void)
{
    setbuf(stdout, NULL);
    printf("\033[32mText with escape sequences\033[0m\n");
    
    return 0;
}
