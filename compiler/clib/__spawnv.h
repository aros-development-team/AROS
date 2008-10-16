#ifndef ___STAT_H
#define ___STAT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stat() internals - header file
    Lang: english
*/

#include <dos/bptr.h>

typedef struct
{
    BPTR command;
    LONG returncode;
    struct arosc_privdata *ppriv;
} childdata_t;

int __spawnv(int mode, BPTR seg, char *const argv[]);

#endif
