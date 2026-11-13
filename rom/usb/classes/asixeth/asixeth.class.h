#ifndef ASIXETH_CLASS_H
#define ASIXETH_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                         Includes for asixeth class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <libraries/gadtools.h>
#include <libraries/mui.h>

#include <devices/newstyle.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "asixeth.h"
#include "dev.h"

/* Protos */

struct NepClassEth * usbAttemptDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
struct NepClassEth * usbForceDeviceBinding(struct NepEthBase *nh, struct PsdDevice *pd);
void usbReleaseDeviceBinding(struct NepEthBase *nh, struct NepClassEth *ncp);

struct NepClassEth * nAllocEth(void);
void nFreeEth(struct NepClassEth *ncp);

BOOL nReadEEPROMMAC(struct NepClassEth *ncp, UBYTE *macptr);
LONG nReadPhyWord(struct NepClassEth *ncp, ULONG phyid, ULONG phyreg);
BOOL nWritePhyWord(struct NepClassEth *ncp, ULONG phyid, ULONG phyreg, ULONG value);

BOOL nInitASIX(struct NepClassEth *ncp);
void nSetOnline(struct NepClassEth *ncp);
void nUpdateRXMode(struct NepClassEth *ncp);

void nDoEvent(struct NepClassEth *ncp, ULONG events);
BOOL nWritePacket(struct NepClassEth *ncp, struct IOSana2Req *ioreq);
BOOL nReadPacket(struct NepClassEth *ncp, UBYTE *pktptr, ULONG len);

BOOL nLoadClassConfig(struct NepEthBase *nh);
BOOL nLoadBindingConfig(struct NepClassEth *ncp);
LONG nOpenBindingCfgWindow(struct NepEthBase *nh, struct NepClassEth *ncp);

void nGUITaskCleanup(struct NepClassEth *nh);

AROS_UFP0(void, nEthTask);
AROS_UFP0(void, nGUITask);

#endif /* ASIXETH_CLASS_H */
