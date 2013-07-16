/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <netdb.h>
#include <string.h>

#include "winsock2.h"
#include "netdb_util.h"

char *CopyString(char *src, APTR pool)
{
    char *dst;
    ULONG l = strlen(src) + 1;

    dst = AllocVecPooled(pool, l);
    if (dst)
	CopyMem(src, dst, l);

    return dst;
}

static char **CopyStringArray(char **src, APTR pool)
{
    char **dst;
    ULONG l = 0;

    while (src[l])
    {
	D(bug("[CopyStringArray] Counted element: %s\n", src[l]));
	l++;
    };
    D(bug("[CopyStringArray] %u elements total\n", l));

    dst = AllocVecPooled(pool, (l + 1) * sizeof(char *));
    if (dst)
    {
	ULONG i;

	for (i = 0; i < l; i++)
	{
	    dst[i] = CopyString(src[i], pool);
	    D(bug("[CopyStringArray] Copied element: %s\n", dst[i]));
	    if (!dst[i])
		break;
	}

	/* NULL-terminate the array */
	D(bug("[CopyStringArray] Terminator element: %u\n", i));
	dst[i] = NULL;
    }

    return dst;
}

static void FreeStringArray(char **src, APTR pool)
{
    ULONG i;

    for (i = 0; src[i]; i++)
	FreeVecPooled(pool, src[i]);

    FreeVec(src);
}

struct protoent *CopyProtoEntry(struct PROTOENT *wsentry, APTR pool)
{
    struct protoent *entry = AllocPooled(pool, sizeof(struct protoent));

    D(bug("[CopyProtoEntry] Allocated AROS protoent 0x%p\n", entry));
    if (entry)
    {
	D(bug("[CopyProtoEntry] Original Name: %s, Aliases: 0x%p\n", wsentry->p_name, wsentry->p_aliases));
	entry->p_name = CopyString(wsentry->p_name, pool);
	D(bug("[CopyProtoEntry] Copied name: 0x%p (%s)\n", entry->p_name, entry->p_name));
	if (entry->p_name)
	{
	    entry->p_aliases = CopyStringArray(wsentry->p_aliases, pool);
	    D(bug("[CopyProtoEntry] Copied alias table: 0x%p\n", entry->p_aliases));
	    if (entry->p_aliases)
	    {
		entry->p_proto = wsentry->p_proto;

		return entry;
	    }
	}

	/* Free the incomplete entry */
	FreeProtoEntry(entry, pool);
    }

    return NULL;
}

void FreeProtoEntry(struct protoent *entry, APTR pool)
{
    if (entry->p_aliases)
	FreeStringArray(entry->p_aliases, pool);

	FreeVecPooled(entry->p_name, pool);

    FreePooled(pool, entry, sizeof(struct protoent));
}
