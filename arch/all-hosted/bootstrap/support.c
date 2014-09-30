/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "support.h"

char *DefaultConfig = ARCH "/AROSBootstrap.conf";

char *getosversion(const char *version)
{
    char *parts[] =
    {
        version,
        "/",
        "Unknown operating system"
    }
    
    return join_strings(3, parts);
}

char *namepart(char *name)
{
    while (*name)
	name++;

    while((name[-1] != ':') && (name[-1] != '\\') && (name[-1] != '/'))
	name--;

    return name;
}
