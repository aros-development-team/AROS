/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - arosc.library specific code
    Lang: english
*/

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/exec.h>
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
    AROS_GET_SYSBASE
    struct AroscUserData *userdata;
    int ret;

    userdata = (struct AroscUserData *)AllocVec(sizeof(struct AroscUserData), MEMF_ANY|MEMF_CLEAR);
    if (!userdata)
        return RETURN_FAIL;

    /* passess these values to the library */
    userdata->errnoptr           = &errno;
    userdata->stdinptr           = &stdin;
    userdata->stdoutptr          = &stdout;
    userdata->stderrptr          = &stderr;
    userdata->startup_jmp_bufptr = &__startup_jmp_buf;
    userdata->startup_errorptr   = &__startup_error;

    /*
      This will be useful in case the structure will be extended.
      In this way the library will always know what can "offer" to the
      program
    */
    userdata->usersize           = sizeof(struct AroscUserData);

    /* Tell the library it can now initialize its internal stuff */
    ret = syscall(arosc_internalinit, userdata);

    if (!ret)
    {
        /*get these values from the library */
        __ctype_b       = userdata->ctype_b;
        __ctype_toupper = userdata->ctype_toupper;
        __ctype_tolower = userdata->ctype_tolower;
    }

    return ret;
}

static void preclose(void)
{
    /*
      Let the library free the memory we allocated.
      It might not seem a good thing, but in this way
      we can avoid being aware what does the library do
      with the memory we've allocated and so this code
      will always work
    */
    syscall(arosc_internalexit);
}

ADD2LIBS(AROSCNAME, 39, LIBSET_AROSC_PRI, struct Library *, aroscbase, postopen, preclose);
