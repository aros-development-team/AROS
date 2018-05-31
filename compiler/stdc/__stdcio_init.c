/*
    Copyright © 2012-2018, The AROS Development Team. All rights reserved.
    $Id$

    Initialisation code for stdcio.library
*/
#include <aros/symbolsets.h>

#include <proto/stdcio.h>

/* Set StdCBase offset */
const ULONG __aros_rellib_base_StdCBase = 0;
SETRELLIBOFFSET(StdCBase, struct StdCIOBase, StdCBase)
