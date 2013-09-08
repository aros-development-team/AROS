#ifndef _LIBRARIES_STDC_H
#define _LIBRARIES_STDC_H

/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Public part of StdC libbase.
    Take care of backwards compatibility when changing something in this file.
*/

#include <exec/libraries.h>

#include <setjmp.h>

struct StdCBase
{
    struct Library lib;

    /* ctype.h */
    const unsigned short int * __ctype_b;
    const unsigned char * __ctype_toupper;
    const unsigned char * __ctype_tolower;

    /* errno.h */
    int _errno;

    /* math.h */
    int _signgam;

    /* stdlib.h */
    int mb_cur_max;
};

__BEGIN_DECLS

struct StdCBase *__aros_getbase_StdCBase(void);

/* Some internal support functions */
void __stdc_program_startup(jmp_buf exitjmp, int *errorptr);
void __stdc_program_end(void);
int *__stdc_set_errorptr(int *errorptr);
int *__stdc_get_errorptr(void);
void __stdc_set_exitjmp(jmp_buf exitjmp, jmp_buf previousjmp);
void __stdc_jmp2exit(int normal, int returncode) __noreturn;

__END_DECLS

#endif /* _LIBRARIES_STDC_H */
