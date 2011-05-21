/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include "Shell.h"

LONG cliLen(CONST_STRPTR s)
{
    LONG i;

    for (i = 0; *s != '\0'; ++i)
	++s;

    return i;
}

