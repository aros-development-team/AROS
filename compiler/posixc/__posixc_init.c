/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    Internal initialisation code for posixc.library
*/

#include <aros/symbolsets.h>

#include <libraries/posixc.h>

/* We handle StdCBase */
const ULONG const __aros_rellib_base_StdCBase = 0;
SETRELLIBOFFSET(StdCBase, struct PosixCBase, StdCBase)

/* We handle StdCIOBase */
const ULONG const __aros_rellib_base_StdCIOBase = 0;
SETRELLIBOFFSET(StdCIOBase, struct PosixCBase, StdCIOBase)
