/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include "debug.h"
#include "posinfo.h"

void Purify_AssertFailed (const char * file, int line, const char * fname,
			const char * expr)
{
    fprintf (stderr, "%s:%d: %s(): Assertion `%s' failed.\n",
	file, line, fname, expr
    );
    fprintf (stderr, "This was inserted at %s:%d into %s()\n",
	Purify_Filename, Purify_Lineno, Purify_Functionname
    );
    abort ();
}

