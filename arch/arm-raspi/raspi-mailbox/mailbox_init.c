/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/atomic.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include "mailbox_private.h"

#include <aros/debug.h>

#include LC_LIBDEFS_FILE

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR lh) {
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
