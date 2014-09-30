/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include <stdio.h>
#include "util.h"

void * xmalloc (int size)
{
    void * ptr = malloc (size);

    if (!ptr)
    {
	fprintf (stderr, "Out of memory\n");
	exit (20);
    }

    return ptr;
}

void xfree (void * ptr)
{
    if (ptr)
	free (ptr);
    else
	fprintf (stderr, "Warning: free (NULL);\n");
}
