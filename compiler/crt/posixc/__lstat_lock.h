/*
 * Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Distributed under the AROS Public License 1.1.
 */
#ifndef POSIXC_LSTAT_LOCK_H
#define POSIXC_LSTAT_LOCK_H

#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>

/* Lock() follows soft links. Resolve the parent with Lock(), then send one
 * raw locate packet for the final component. ERROR_IS_SOFT_LINK is deliberately
 * returned to lstat's existing directory-entry fallback, including dangling
 * links. Intermediate links and assigns retain normal DOS resolution. */
static BPTR __lstat_lock(const char *name, LONG accessMode)
{
    const char *leaf = FilePart(name);
    char *parent;
    BPTR parent_lock, result;
    LONG error;
    size_t parent_len;
    BSTR packet_name;
#ifndef AROS_FAST_BSTR
    union { IPTR alignment; unsigned char bytes[257]; } bname;
    size_t leaf_len;
#endif

    /* A root/volume/empty name has no final entry to dereference. */
    if (!*leaf)
        return Lock(name, accessMode);
#ifndef AROS_FAST_BSTR
    leaf_len = strlen(leaf);
    if (leaf_len > 255) {
        SetIoErr(ERROR_LINE_TOO_LONG);
        return BNULL;
    }
    bname.bytes[0] = leaf_len;
    memcpy(bname.bytes + 1, leaf, leaf_len + 1);
    packet_name = (BSTR)MKBADDR(bname.bytes);
#else
    packet_name = (BSTR)leaf;
#endif
    parent_len = leaf - name;
    parent = AllocVec(parent_len + 1, MEMF_ANY);
    if (!parent) {
        SetIoErr(ERROR_NO_FREE_STORE);
        return BNULL;
    }
    memcpy(parent, name, parent_len);
    parent[parent_len] = '\0';
    parent_lock = Lock(parent, SHARED_LOCK);
    error = IoErr();
    FreeVec(parent);
    if (!parent_lock) {
        SetIoErr(error);
        return BNULL;
    }
    result = (BPTR)DoPkt(((struct FileLock *)BADDR(parent_lock))->fl_Task,
                         ACTION_LOCATE_OBJECT, parent_lock, (SIPTR)packet_name,
                         accessMode, 0, 0);
    error = IoErr();
    UnLock(parent_lock);
    SetIoErr(error);
    return result;
}
#endif
