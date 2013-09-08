/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <libraries/stdc.h>

const unsigned short int * const *__ctype_b_ptr = NULL;
const unsigned char * const *__ctype_toupper_ptr = NULL;
const unsigned char * const *__ctype_tolower_ptr = NULL;

static struct StdCBase *_StdCBase;
static int opened = 0;

static int __ctype_init(struct ExecBase *SysBase)
{
    _StdCBase = __aros_getbase_StdCBase();
    /* Be sure ctype functions may be called from init code.
     * If library is using relbase version of stdc.library it will
     * only open stdc.library during OpenLib and _StdCBase may
     * be NULL at this point.
     * Try to open stdc.library manually here
     */
    if (!_StdCBase)
    {
        _StdCBase = (struct StdCBase *)OpenLibrary("stdc.library", 0);
        opened = 1;
    }
    if (!_StdCBase)
        return 0;

    __ctype_b_ptr = &_StdCBase->__ctype_b;
    __ctype_toupper_ptr = &_StdCBase->__ctype_toupper;
    __ctype_tolower_ptr = &_StdCBase->__ctype_tolower;

    return 1;
}

static void __ctype_exit(void)
{
    if (opened && _StdCBase)
        CloseLibrary((struct Library *)_StdCBase);
}

ADD2INIT(__ctype_init, 20);
ADD2EXIT(__ctype_exit, 20);
