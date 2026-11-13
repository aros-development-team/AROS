/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library packet helpers (FS enforcer)
*/
#ifndef _SECURITY_PACKETIO_H
#define _SECURITY_PACKETIO_H

#include <dos/dosextens.h>

struct SecurityBase;
struct secVolume;

extern struct secExtOwner *GetPktOwner(struct SecurityBase *secBase, struct DosPacket *pkt);
extern struct Task *GetPktTask(struct DosPacket *pkt);
extern LONG GetPktDefProtection(struct SecurityBase *secBase, struct DosPacket *pkt);

extern SIPTR secFSDoPkt(struct SecurityBase *secBase, struct secVolume *Vol, LONG act, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5, SIPTR *res2);
extern SIPTR DoPacket(struct SecurityBase *secBase, struct secVolume *Vol, struct DosPacket *pkt);

#endif /* _SECURITY_PACKETIO_H */
