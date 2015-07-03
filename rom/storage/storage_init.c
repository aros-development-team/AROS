/*
    Copyright 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <exec/memory.h>
#include <aros/symbolsets.h>
#include <string.h>

#include "storage_intern.h"

#include LC_LIBDEFS_FILE

static int InitStorageResource( LIBBASETYPEPTR LIBBASE)
{
    D(bug("[StorageRes] %s()\n", __PRETTY_FUNCTION__));

    NEWLIST(&LIBBASE->sb_IDs);
    NEWLIST(&LIBBASE->sb_Devices);

    D(bug("[StorageRes] %s: Initialized\n", __PRETTY_FUNCTION__));

    return TRUE;
}

ADD2INITLIB(InitStorageResource, 0)
