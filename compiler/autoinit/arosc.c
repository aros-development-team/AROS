#include <aros/symbolsets.h>
#include <stdio.h>
#include <setjmp.h>

#define _CLIB_KERNEL_
#include <libraries/arosc.h>

#undef errno
#undef stdin
#undef stdout
#undef stderr
#undef __startup_jmp_buf
#undef __startup_error

int  errno;
FILE *stdin, *stdout, *stderr;

extern jmp_buf __startup_jmp_buf;
extern LONG __startup_error;


static int postopen(void)
{
    GETUSER;

    clib_userdata->errnoptr           = &errno;
    clib_userdata->stdinptr           = &stdin;
    clib_userdata->stdoutptr          = &stdout;
    clib_userdata->stderrptr          = &stderr;
    clib_userdata->startup_jmp_bufptr = &__startup_jmp_buf;
    clib_userdata->startup_errorptr   = &__startup_error;

    return arosc_internalinit();
}

static void preclose(void)
{
    arosc_internalexit();
}

ADDLIB2SET(AROSCNAME, 39, struct Library *, aroscbase, postopen, preclose);
