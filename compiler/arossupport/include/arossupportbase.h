#ifndef AROS_AROSSUPPORTBASE_H
#define AROS_AROSSUPPORTBASE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

struct AROSSupportBase
{
    void * StdOut;
    int (*kprintf)(const char *, ...);
    int (*rkprintf)(const char *, const char *, int, const char *, ...);
    void * DebugConfig;
};

#endif /* AROS_AROSSUPPORTBASE_H */
