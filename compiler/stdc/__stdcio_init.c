/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Initialisation code for stdcio.library
*/
#include <aros/symbolsets.h>

#include <proto/stdcio.h>

/* Set StdCBase offset */
const ULONG const __aros_rellib_base_StdCBase = 0;
SETRELLIBOFFSET(StdCBase, struct StdCIOBase, StdCBase)
