/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/arosc.h>
#include <aros/symbolsets.h>

const unsigned short int *__ctype_b;
const int *__ctype_toupper;
const int *__ctype_tolower;

static int __ctype_init(void)
{
    const struct arosc_ctype *ctype = __get_arosc_ctype();

    __ctype_b = ctype->b;
    __ctype_toupper = ctype->toupper;
    __ctype_tolower = ctype->tolower;

    return 1;
}

ADD2INIT(__ctype_init, 20);
