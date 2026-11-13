/*
 * misc.c
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#include <proto/exec.h>

#include "base.h"

/*
 * Free nodes of a list (they should all be allocated with AllocVec())
 */
static inline void FreeListVec(struct NetInfoDevice *nid, struct List *list)
{
    struct Node *entry, *next;

    for (entry = list->lh_Head; next = entry->ln_Succ; entry = next) {
        Remove(entry);
        FreeVec(entry);
        entry = NULL;
    }
}

char *strsep(register char **stringp, register const char *delim);
