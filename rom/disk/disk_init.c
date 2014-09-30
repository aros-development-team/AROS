/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#include <proto/disk.h>
#include <resources/disk.h>

#include <aros/symbolsets.h>
#include "disk_intern.h"

extern BOOL disk_internal_init (struct DiscResource*);

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR DiscResource)
{
    return disk_internal_init(DiscResource);
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
