/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/arosc.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include "__arosc_privdata.h"

const unsigned short int *__ctype_b;
const unsigned char *__ctype_toupper;
const unsigned char *__ctype_tolower;

struct aroscbase *_aroscbase;

static int __ctype_init(void)
{
    const struct arosc_ctype *ctype;

    _aroscbase = (struct aroscbase *)OpenLibrary("arosc.library", 0);
    if (!_aroscbase)
    	    return 0;
    ctype = &_aroscbase->acb_acud.acud_ctype;

    __ctype_b = ctype->b;
    __ctype_toupper = ctype->toupper;
    __ctype_tolower = ctype->tolower;

    return 1;
}

static void __ctype_exit(void)
{
    CloseLibrary((struct Library *)_aroscbase);
}

ADD2INIT(__ctype_init, 20);
ADD2EXIT(__ctype_exit, 20);
