/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    This file defines the private part of StdCBase.
    This should only be used internally in stdc.library code so
    changes can be made to this structure without breaking backwards
    compatibility.
*/
#ifndef __STDC_INTBASE_H
#define __STDC_INTBASE_H

#include <libraries/stdc.h>
#include <devices/timer.h>

#include <time.h>

#include <aros/types/clock_t.h>


struct StdCIntBase
{
    struct StdCBase StdCBase;

    /* stdlib.h */
    APTR mempool;
    unsigned int srand_seed;

    /* time.h and it's functions */
    struct timerequest timereq;
    struct MsgPort timeport;
    char timebuffer[26];
    struct tm tmbuffer;
    clock_t starttime;
};

#endif //__STDC_INTBASE_H
