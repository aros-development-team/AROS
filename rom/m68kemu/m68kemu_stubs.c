/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/* Stubs for C library functions Moira needs in kernel context */
#include <aros/debug.h>
#include <exec/types.h>

struct timeval { long tv_sec; long tv_usec; };

int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tv; (void)tz;
    return 0;
}

typedef int jmp_buf_t[64];
void longjmp(jmp_buf_t env, int val)
{
    (void)env; (void)val;
    bug("[m68kemu] longjmp called — should never be reached\n");
}
