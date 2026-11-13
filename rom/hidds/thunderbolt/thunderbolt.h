#ifndef _THUNDERBOLT_H
#define _THUNDERBOLT_H

/*
    Copyright © 2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/bptr.h>

#include <oop/oop.h>

#include LC_LIBDEFS_FILE

struct thunderbolt_staticdata {
    struct Library      *oopBase;
    OOP_Class           *thunderboltClass;
    BPTR                segList;
};

struct thunderboltbase {
    struct Library              LibNode;
    struct thunderbolt_staticdata psd;
};

#define BASE(lib)               ((struct thunderboltbase *)(lib))

#endif
