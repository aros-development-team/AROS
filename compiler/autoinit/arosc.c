/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <aros/symbolsets.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/syscall.h>
#include <libraries/arosc.h>

int  errno;
FILE *stdin, *stdout, *stderr;

unsigned short int * __ctype_b;
int * __ctype_toupper;
int * __ctype_tolower;

/* in the startup code */
extern jmp_buf __startup_jmp_buf;
extern LONG __startup_error;

static int postopen(void)
{
    struct AroscUserdata *userdata = (struct AroscUserdata *)(FindTask(0)->tc_UserData);

    /* passess these values to the library */
    userdata->errnoptr           = &errno;
    userdata->stdinptr           = &stdin;
    userdata->stdoutptr          = &stdout;
    userdata->stderrptr          = &stderr;
    userdata->startup_jmp_bufptr = &__startup_jmp_buf;
    userdata->startup_errorptr   = &__startup_error;

    /*get these values from the library */
    __ctype_b       = userdata->ctype_b;
    __ctype_toupper = userdata->ctype_toupper;
    __ctype_tolower = userdata->ctype_tolower;

    /* Tell the library it can now initialize its internal stuff */
    return syscall(arosc_internalinit);
}

static void preclose(void)
{
    syscall(arosc_internalexit);
}

ADD2LIBS(AROSCNAME, 39, LIBSET_AROSC_PRI, struct Library *, aroscbase, postopen, preclose);
