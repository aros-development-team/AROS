/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library monitors
*/
#ifndef _SECURITY_MONITOR_H
#define _SECURITY_MONITOR_H

#include <exec/types.h>

struct SecurityBase;

extern void InitMonList(struct SecurityBase *secBase);
extern void CallMonitors(struct SecurityBase *secBase, ULONG triggerbit, UWORD from, UWORD to, CONST_STRPTR userid);
extern void FreeRepliedMonMsg(struct SecurityBase *secBase);

#endif /* _SECURITY_MONITOR_H */
