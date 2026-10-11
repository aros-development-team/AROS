/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    libssh_net_aros.c - network helpers AROS does not provide as plain
    functions.

    if_nametoindex() exists only as a miami.library vector on AROS; open it
    lazily and fall back to 0 ("no such interface") without it, which libssh
    treats as a failed scoped-address lookup.
*/

#include "config.h"

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/libcall.h>

/* miami.library vector 46 (clib/miami_protos.h), called directly so this
 * file's POSIX prototype does not clash with the header's own */
#define MIAMI_IF_NAMETOINDEX(base, name) \
    AROS_LC1(LONG, if_nametoindex, \
             AROS_LCA(char *, (name), A0), \
             struct Library *, (base), 46, Miami)

unsigned int if_nametoindex(const char *ifname)
{
    struct Library *MiamiBase;
    LONG idx = 0;

    MiamiBase = OpenLibrary("miami.library", 0);
    if (MiamiBase != NULL) {
        idx = MIAMI_IF_NAMETOINDEX(MiamiBase, (char *)ifname);
        CloseLibrary(MiamiBase);
    }
    return (unsigned int)idx;
}
