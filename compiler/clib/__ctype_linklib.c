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

struct aroscbase *Arosc_GetLibbase(void);

static int __ctype_init(void)
{
    
    const struct arosc_ctype *ctype;
    struct aroscbase *_aroscbase;

    _aroscbase = Arosc_GetLibbase();
    /* FIXME: __ctype_init is linked statically in external code but
       is dependent on internal library struct
    */
    if (_aroscbase)
        ctype = &_aroscbase->acb_acud.acud_ctype;
    else
    {
        _aroscbase = (struct aroscbase *)OpenLibrary("arosc.library", 0);
        if (!_aroscbase)
            return 0;
        ctype = &_aroscbase->acb_acud.acud_ctype;
        CloseLibrary((struct Library *)_aroscbase);
    }

    __ctype_b = ctype->b;
    __ctype_toupper = ctype->toupper;
    __ctype_tolower = ctype->tolower;

    return 1;
}

ADD2INIT(__ctype_init, 20);
