/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function prototypes for functions to call from the query function
    Lang: English
*/

#include <exec/types.h>

LONG rxsupp_allocmem(struct Library *, struct RexxMsg *, UBYTE **);
LONG rxsupp_freemem(struct Library *, struct RexxMsg *, UBYTE **);
