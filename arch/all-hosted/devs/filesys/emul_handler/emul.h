#ifndef RESOURCES_EMUL_H
#define RESOURCES_EMUL_H
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Resource base contents for emulation-handler.
    Lang: english
*/

#include <dos/dosextens.h>

struct EmulHandler
{
    struct Node	eb_Node;
    APTR	eb_stdin;
    APTR	eb_stdout;
    APTR	eb_stderr;
};

#endif /* RESOURCES_EMUL_H */
