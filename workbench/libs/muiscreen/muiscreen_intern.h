#ifndef PARTITION_INTERN_H
#define PARTITION_INTERN_H

/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/muiscreen.h>

#include <aros/libcall.h>
#include LC_LIBDEFS_FILE

struct MUIScreenBase_intern
{
    struct Library lib;
    struct List clients;
};

#endif
