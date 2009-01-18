/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test program for the arosc's setjmp() function.
*/
#include <setjmp.h>
#include <stdio.h>

jmp_buf env;
int i = 0;

int main(void)
{
    int a = 1234;

    printf("a:%d, &a: %p\n", a, &a);
    switch(setjmp(env))
    {
        case 0:
        printf("setjmp 0, i = %d, a: %d, &a: %p\n", i, a, &a);
        i++;
        longjmp(env, i);
        
        case 1:
        printf("setjmp 1, i = %d, a: %d, &a: %p\n", i, a, &a);
        i++;
        longjmp(env, i);

        case 2:
        printf("setjmp 2, i = %d, a: %d, &a: %p\n", i, a, &a);
        break;
        
        default:
        printf("Something's very wrong !!!\n");
        return 20;
        break;
    }
    
    return 0;
}
