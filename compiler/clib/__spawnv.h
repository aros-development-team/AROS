#ifndef ___STAT_H
#define ___STAT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: stat() internals - header file
    Lang: english
*/

#include <dos/bptr.h>

int __spawnv(int mode, BPTR seg, char *const argv[]);

#endif
