/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/system.h>
#include <aros/symbolsets.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "parallel_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>
