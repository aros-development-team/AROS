/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/arosc.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include "__arosc_privdata.h"

void *__aros_getbase_aroscbase(void);

const unsigned short int *__ctype_b;
const unsigned char *__ctype_toupper;
const unsigned char *__ctype_tolower;

static int __ctype_init(struct ExecBase *SysBase)
{
    const struct arosc_ctype *ctype;
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    if (!aroscbase)
    	    return 0;

    ctype = &aroscbase->acb_acud.acud_ctype;

    __ctype_b = ctype->b;
    __ctype_toupper = ctype->toupper;
    __ctype_tolower = ctype->tolower;

    return 1;
}

ADD2INIT(__ctype_init, 20);
