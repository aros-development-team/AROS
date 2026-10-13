/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library segment (setuid executable) tracking
*/
#ifndef _SECURITY_SEGMENT_H
#define _SECURITY_SEGMENT_H

#include <exec/lists.h>
#include <dos/dos.h>
#include <libraries/security.h>

struct SecurityBase;

/* Private Segment Node */
struct secSegNode
{
    struct MinNode      Node;
    BPTR                SegList;
    struct secExtOwner  Owner;          /* Only uid/gid, no secondary gids */
};

extern void InitSegList(struct SecurityBase *secBase);

/* These obtain SegOwnerSem themselves */
extern BOOL AddSegNode(struct SecurityBase *secBase, BPTR seglist, ULONG owner);
extern void RemSegNode(struct SecurityBase *secBase, BPTR seglist);
/* Returns TRUE and fills *owner if the seglist is a setuid executable */
extern BOOL GetSegOwner(struct SecurityBase *secBase, BPTR seglist, struct secExtOwner *owner);

#endif /* _SECURITY_SEGMENT_H */
